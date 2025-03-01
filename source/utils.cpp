#include <chrono>
#include <fstream>
#include <map>
#include "utils.h"


namespace utils {
	using namespace std;
	using namespace std::chrono;

	static const auto tz = current_zone();
	static constexpr string_view tzFmt("{:%Y/%m/%d %H:%M:%S}");

	string UTCString() {
		const auto utcNow = system_clock::now();
		return format(tzFmt, floor<seconds>(utcNow));
	}

	string UTCString_ms() {
		const auto utcNow = system_clock::now();
		return format(tzFmt, utcNow);
	}

	string TzString() {
		const auto utcNow = system_clock::now();
		const auto tzNow = tz->to_local(utcNow);
		return format(tzFmt, floor<seconds>(tzNow));
	}

	string TzString_ms() {
		const auto utcNow = system_clock::now();
		const auto tzNow = tz->to_local(utcNow);
		return format(tzFmt, tzNow);
	}

	//
	//
	//

	static map<int, ofstream> logfileMap;
	static bool logFailed = false;

	void Log(string_view msg, const int channel) {
		string filename;
		if (!logfileMap.contains(channel)) {
			if (!logFailed) {
				if (channel == -1) {
					filename = format("{:s}.txt", rsc_Name);
				}
				else {
					filename = format("{:s}{:s}{:d}.txt", rsc_Name, "_Debug", channel);
				}
				logfileMap[channel] = ofstream(filename, ios::out | ios::binary | ios::trunc);
				if (!logfileMap[channel].is_open()) {
					logFailed = true; // Couldn't open file for writing, better not try again
				}
			}
		}
		else {
			if ((!logfileMap[channel].is_open() && !logFailed)) {
				if (channel == -1) {
					filename = format("{:s}.txt", rsc_Name);
				}
				else {
					filename = format("{:s}{:s}{:d}.txt", rsc_Name, "_Debug", channel);
				}
				logfileMap[channel] = ofstream(filename, ios::out | ios::binary | ios::app);
				if (!logfileMap[channel].is_open()) {
					logFailed = true; // Couldn't open file for writing, better not try again
				}
			}
		}
		if (!msg.empty() && !logFailed) {
			logfileMap[channel] << msg << endl;
		}
	}

	void Log(string_view msg, const bool close, const int channel) {
		Log(msg, channel);
		if (close && logfileMap.contains(channel) && logfileMap[channel].is_open()) {
			logfileMap[channel].close();
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
