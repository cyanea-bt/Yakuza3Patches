#include <filesystem>
#include <limits>
#include <ostream>
#include <nlohmann/json.hpp>


namespace config {
	using namespace std;
	namespace fs = std::filesystem;
	using json = nlohmann::ordered_json;

	template<typename T>
	T jsonToNumber(json::const_reference element, const T minVal, const T defaultVal) {
		T retVal;
		const T MAX = numeric_limits<T>::max();
		const T MIN = numeric_limits<T>::min();
		if (element.is_number_unsigned()) {
			uint64_t temp = 0;
			element.get_to(temp);
			temp = temp > MAX ? MAX : temp;
			temp = temp < MIN ? MIN : temp;
			temp = temp >= minVal ? temp : defaultVal;
			retVal = static_cast<T>(temp);
		}
		else if (element.is_number_integer()) {
			int64_t temp = 0;
			element.get_to(temp);
			temp = temp > MAX ? MAX : temp;
			temp = temp < MIN ? MIN : temp;
			temp = temp >= minVal ? temp : defaultVal;
			retVal = static_cast<T>(temp);
		}
		else if (element.is_string()) {
			string temp;
			element.get_to(temp);
			// ref: https://stackoverflow.com/a/64092063
			auto [ptr, ec] = from_chars(temp.data(), temp.data() + temp.size(), retVal);
			if (ec != errc{}) {
				// conversion failed
				retVal = defaultVal;
			}
		}
		else {
			retVal = defaultVal;
		}
		return retVal;
	}

	bool jsonToBool(json::const_reference element, const bool defaultVal);
	string jsonToString(json::const_reference element, const string &defaultVal);
	bool writeJson(ostream &os, const json &j);
	void saveConfig(const fs::path configPath, const Config newConfig);
	Config loadConfig();
}
