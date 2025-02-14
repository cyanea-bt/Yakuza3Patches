#include <chrono>
#include <format>
#include <fstream>
#include <map>


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
}
