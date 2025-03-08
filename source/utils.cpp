#include <chrono>
#include <fstream>
#include <map>
#include "utils.h"

#if _DEBUG
static constexpr bool isDEBUG = true;
#else
static constexpr bool isDEBUG = false;
#endif // _DEBUG


namespace utils {
	using namespace std;
	using namespace std::chrono;

	static const auto tz = current_zone();
	static constexpr auto tzFmt = FMT_COMPILE("{:%Y/%m/%d - %H:%M:%S}");
	static constexpr auto tzFileFmt = FMT_COMPILE("{:%Y.%m.%d_%H-%M-%S}");

	string UTCString() {
		const auto utcNow = system_clock::now();
		return fmt::format(tzFmt, floor<seconds>(utcNow));
	}

	string UTCString_ms() {
		const auto utcNow = system_clock::now();
		return fmt::format(tzFmt, utcNow);
	}

	string TzString() {
		const auto utcNow = system_clock::now();
		const auto tzNow = tz->to_local(utcNow);
		return fmt::format(tzFmt, floor<seconds>(tzNow));
	}

	string TzString_ms() {
		const auto utcNow = system_clock::now();
		const auto tzNow = tz->to_local(utcNow);
		return fmt::format(tzFmt, tzNow);
	}

	string UTCFilename() {
		const auto utcNow = system_clock::now();
		return fmt::format(tzFileFmt, floor<seconds>(utcNow));
	}

	string UTCFilename_ms() {
		const auto utcNow = system_clock::now();
		return fmt::format(tzFileFmt, utcNow);
	}

	string TzFilename() {
		const auto utcNow = system_clock::now();
		const auto tzNow = tz->to_local(utcNow);
		return fmt::format(tzFileFmt, floor<seconds>(tzNow));
	}

	string TzFilename_ms() {
		const auto utcNow = system_clock::now();
		const auto tzNow = tz->to_local(utcNow);
		return fmt::format(tzFileFmt, tzNow);
	}

	//
	//
	//

	static map<int, shared_ptr<spdlog::logger>> s_LogfileMap;
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

	//
	//
	//

	// ref: https://gist.github.com/GenesisFR/cceaf433d5b42dcdddecdddee0657292
	string replaceAll(const string &str, const string &from, const string &to) {
		string newStr(str);
		size_t start_pos = 0;
		while ((start_pos = newStr.find(from, start_pos)) != string::npos) {
			newStr.replace(start_pos, from.length(), to);
			start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
		}
		return newStr;
	}

	void replaceAllByRef(string &str, const string &from, const string &to) {
		size_t start_pos = 0;
		while ((start_pos = str.find(from, start_pos)) != string::npos) {
			str.replace(start_pos, from.length(), to);
			start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
		}
	}


	// ref: https://stackoverflow.com/a/63050738
	string_view ltrim(string_view str)
	{
		const auto pos(str.find_first_not_of(" \t\n\r\f\v"));
		str.remove_prefix(min(pos, str.length()));
		return str;
	}

	string_view rtrim(string_view str)
	{
		const auto pos(str.find_last_not_of(" \t\n\r\f\v"));
		str.remove_suffix(min(str.length() - pos - 1, str.length()));
		return str;
	}

	string_view trim(string_view str)
	{
		str = ltrim(str);
		str = rtrim(str);
		return str;
	}


	// ref: https://en.cppreference.com/w/cpp/string/byte/toupper
	string uppercase(string_view str) {
		stringstream ss;
		for (const unsigned char &c : str) {
			ss << static_cast<char>(std::toupper(c));
		}
		return ss.str();
	}

	string lowercase(string_view str) {
		stringstream ss;
		for (const unsigned char &c : str) {
			ss << static_cast<char>(std::tolower(c));
		}
		return ss.str();
	}
}
