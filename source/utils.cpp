#include <map>
#include "common.h"
#include "utils.h"
#include "fmtlib_headers.h"
#include "spdlog_headers.h"


namespace utils {
	using std::map;

	static map<int, std::shared_ptr<spdlog::logger>> s_LogfileMap;
	static bool s_LogFailed = false;

	void CreateLogger(const int channel, string_view loggerName, const bool truncate) {
		string filename;
		if (channel == -1) {
			filename = fmt::format("{:s}.txt", rsc_Name);
		}
		else {
			filename = fmt::format("{:s}{:s}{:d}.txt", rsc_Name, "_Debug", channel);
		}
		const string name = loggerName.empty() ? filename : string(loggerName);
		try {
			s_LogfileMap[channel] = spdlog::basic_logger_st(name, filename, truncate);
			s_LogfileMap[channel]->set_level(spdlog::level::debug);
			s_LogfileMap[channel]->flush_on(spdlog::level::debug);
			// default (?) pattern: [%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [%s:%#] %v
			// ref: https://github.com/gabime/spdlog/blob/3335c380a0/include/spdlog/pattern_formatter-inl.h#L835
			if (channel == -1) {
				s_LogfileMap[channel]->set_pattern("%v");
			}
			else {
				if (loggerName.empty()) {
					s_LogfileMap[channel]->set_pattern("[%Y/%m/%d %H:%M:%S.%e] %v");
				}
				else {
					auto formatter = std::make_unique<spdlog::pattern_formatter>();
					formatter->add_flag<spdlog::counter_flag<8>>('*').set_pattern("[%Y/%m/%d %H:%M:%S.%e][%n][%*] %v");
					s_LogfileMap[channel]->set_formatter(std::move(formatter));
				}
			}
		}
		catch (const spdlog::spdlog_ex &ex)
		{
			spdlog::error(ex.what());
			if (isDEBUG) {
				DebugBreak();
			}
			s_LogFailed = true; // Log initialization failed
		}
	}

	void Log(string_view msg, const int channel, string_view loggerName) {
		if (!s_LogfileMap.contains(channel)) {
			if (!s_LogFailed) {
				CreateLogger(channel, loggerName, true);
			}
		}
		else {
			if ((s_LogfileMap[channel] == nullptr && !msg.empty() && !s_LogFailed)) {
				CreateLogger(channel, loggerName, false);
			}
		}
		if (!msg.empty() && !s_LogFailed) {
			s_LogfileMap[channel]->info(msg);
		}
	}

	void Log(string_view msg, const bool close, const int channel, string_view loggerName) {
		Log(msg, channel, loggerName);
		if (close && s_LogfileMap.contains(channel) && s_LogfileMap[channel] != nullptr) {
			// Remove logger from spdlog registry
			spdlog::drop(s_LogfileMap[channel]->name());
			// Release our shared_ptr to the logger (which should be the last one pointing to it).
			// All resources for this logger should then be freed and the corresponding log file closed.
			if (isDEBUG) {
				if (s_LogfileMap[channel].use_count() != 1) {
					DebugBreak();
				}
			}
			s_LogfileMap[channel].reset();
		}
	}
}
