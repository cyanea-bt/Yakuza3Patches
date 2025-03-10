#include <chrono>
#include <string>
#include "string_utils.h"
#include "fmtlib_headers.h"


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
