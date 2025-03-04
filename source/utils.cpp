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
	static constexpr auto tzFmt = FMT_COMPILE("{:%Y/%m/%d %H:%M:%S}");

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

	//
	//
	//

	// Custom flag with counter that gets incremented after each use
	// ref: https://github.com/gabime/spdlog/wiki/3.-Custom-formatting
	template <uint8_t minWidth>
	class counter_flag final : public spdlog::custom_flag_formatter {
	public:
		void format(const spdlog::details::log_msg &, const std::tm &, spdlog::memory_buf_t &dest) override {
			constexpr auto formatStringBuf = getFormatString(minWidth);
			const std::string strCounter = fmt::format(formatStringBuf.data(), counter++);
			dest.append(strCounter.data(), strCounter.data() + strCounter.size());
		}

		std::unique_ptr<custom_flag_formatter> clone() const override {
			auto cloned = spdlog::details::make_unique<counter_flag>();
			cloned->counter = counter;
			return cloned;
		}

	private:
		uint64_t counter = 0;

		// evaluate format string at compile time
		// ref: https://www.reddit.com/r/cpp/comments/kpejif/discussion_on_possibility_of_a_compiletime_printf/ghzdo4f/
		//      https://stackoverflow.com/a/68207254
		static consteval auto getFormatString(uint8_t n) {
			auto buf = std::array<char, 16>();
			auto result = fmt::format_to(buf.data(), FMT_COMPILE("{{:0{:d}d}}"), n);
			*result = '\0'; // make sure buf is zero-terminated (should already be the case)
			return buf;
		}
	};

	static map<int, shared_ptr<spdlog::logger>> logfileMap;
	static bool logFailed = false;

	void Log(string_view msg, const int channel, string_view loggerName) {
		string filename;
		if (!logfileMap.contains(channel)) {
			if (!logFailed) {
				if (channel == -1) {
					filename = fmt::format("{:s}.txt", rsc_Name);
				}
				else {
					filename = fmt::format("{:s}{:s}{:d}.txt", rsc_Name, "_Debug", channel);
				}
				const string name = loggerName.empty() ? filename : string(loggerName);
				spdlog::info("Creating logger: \"{:s}\"", name); // for testing, will be removed
				try {
					logfileMap[channel] = spdlog::basic_logger_st(name, filename, true); // truncate
					logfileMap[channel]->set_level(spdlog::level::debug);
					logfileMap[channel]->flush_on(spdlog::level::debug);
					// default (?) pattern: [%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [%s:%#] %v
					// ref: https://github.com/gabime/spdlog/blob/3335c380a0/include/spdlog/pattern_formatter-inl.h#L835
					if (channel == -1) {
						logfileMap[channel]->set_pattern("%v");
					}
					else {
						if (loggerName.empty()) {
							logfileMap[channel]->set_pattern("[%Y/%m/%d %H:%M:%S.%e] %v");
						}
						else {
							auto formatter = std::make_unique<spdlog::pattern_formatter>();
							formatter->add_flag<counter_flag<8>>('*').set_pattern("[%Y/%m/%d %H:%M:%S.%e][%n][%*] %v");
							logfileMap[channel]->set_formatter(std::move(formatter));
						}
					}
				}
				catch (const spdlog::spdlog_ex &ex)
				{
					spdlog::error(ex.what());
					if (isDEBUG) {
						DebugBreak();
					}
					logFailed = true; // Log initialization failed
				}
			}
		}
		else {
			if ((logfileMap[channel] == nullptr && !msg.empty() && !logFailed)) {
				if (channel == -1) {
					filename = fmt::format("{:s}.txt", rsc_Name);
				}
				else {
					filename = fmt::format("{:s}{:s}{:d}.txt", rsc_Name, "_Debug", channel);
				}
				const string name = loggerName.empty() ? filename : string(loggerName);
				spdlog::warn("Re-creating logger: \"{:s}\"", name); // for testing, will be removed
				try {
					logfileMap[channel] = spdlog::basic_logger_st(name, filename, false); // append
					logfileMap[channel]->set_level(spdlog::level::debug);
					logfileMap[channel]->flush_on(spdlog::level::debug);
					if (channel == -1) {
						logfileMap[channel]->set_pattern("%v");
					}
					else {
						if (loggerName.empty()) {
							logfileMap[channel]->set_pattern("[%Y/%m/%d %H:%M:%S.%e] %v");
						}
						else {
							auto formatter = std::make_unique<spdlog::pattern_formatter>();
							formatter->add_flag<counter_flag<8>>('*').set_pattern("[%Y/%m/%d %H:%M:%S.%e][%n][%*] %v");
							logfileMap[channel]->set_formatter(std::move(formatter));
						}
					}
				}
				catch (const spdlog::spdlog_ex &ex)
				{
					spdlog::error(ex.what());
					if (isDEBUG) {
						DebugBreak();
					}
					logFailed = true; // Log initialization failed
				}
			}
		}
		if (!msg.empty() && !logFailed) {
			logfileMap[channel]->info(msg);
		}
	}

	void Log(string_view msg, const bool close, const int channel, string_view loggerName) {
		Log(msg, channel, loggerName);
		if (close && logfileMap.contains(channel) && logfileMap[channel] != nullptr) {
			// Remove logger from spdlog registry
			spdlog::drop(logfileMap[channel]->name());
			// Release our shared_ptr to the logger (which should be the last one pointing to it).
			// All resources for this logger should then be freed and the corresponding log file closed.
			if (isDEBUG) {
				if (logfileMap[channel].use_count() != 1) {
					DebugBreak();
				}
			}
			logfileMap[channel].reset();
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
