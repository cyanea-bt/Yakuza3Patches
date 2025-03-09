#pragma once
#include <filesystem>
#include <limits>
#include <ostream>
#include <nlohmann/json.hpp>
#include "utils.h"


namespace config {
	using namespace std;
	namespace fs = std::filesystem;
	using json = nlohmann::ordered_json;

	template<typename T>
	T jsonToNumber(const json &jsonData, string_view jsonKeyVal, const T minVal, const T maxVal, const T defaultVal) {
		if (!jsonData.contains(jsonKeyVal)) {
			return defaultVal;
		}
		const auto element = jsonData[jsonKeyVal];
		T retVal;
		const T MAX = numeric_limits<T>::max();
		const T MIN = numeric_limits<T>::min();
		if (element.is_number_unsigned()) {
			uint64_t temp = 0;
			element.get_to(temp);
			// check numeric limits of return type
			temp = temp > MAX ? MAX : temp;
			temp = temp < MIN ? MIN : temp;
			// check limits from parameters
			temp = temp >= minVal ? temp : defaultVal;
			temp = temp <= maxVal ? temp : defaultVal;
			retVal = static_cast<T>(temp);
		}
		else if (element.is_number_integer()) {
			int64_t temp = 0;
			element.get_to(temp);
			temp = temp > MAX ? MAX : temp;
			temp = temp < MIN ? MIN : temp;
			temp = temp >= minVal ? temp : defaultVal;
			temp = temp <= maxVal ? temp : defaultVal;
			retVal = static_cast<T>(temp);
		}
		else if (element.is_number_float()) {
			float temp = 0.0f;
			element.get_to(temp);
			temp = temp > MAX ? MAX : temp;
			temp = temp < MIN ? MIN : temp;
			temp = temp >= minVal ? temp : defaultVal;
			temp = temp <= maxVal ? temp : defaultVal;
			retVal = static_cast<T>(temp);
		}
		else if (element.is_string()) {
			// in case someone tries to use a string for a number
			string tempStr;
			element.get_to(tempStr);
			tempStr = utils::trim(tempStr);
			from_chars_result result;
			if (tempStr.find(".") != string::npos || tempStr.find(",") != string::npos) {
				float temp = 0.0f;
				// ref: https://stackoverflow.com/a/64092063
				result = from_chars(tempStr.data(), tempStr.data() + tempStr.size(), temp);
				if (result.ec != errc{}) {
					retVal = defaultVal; // conversion failed
				}
				temp = temp > MAX ? MAX : temp;
				temp = temp < MIN ? MIN : temp;
				temp = temp >= minVal ? temp : defaultVal;
				temp = temp <= maxVal ? temp : defaultVal;
				retVal = static_cast<T>(temp);
			}
			else if (tempStr.starts_with("-")) {
				int64_t temp = 0;
				result = from_chars(tempStr.data(), tempStr.data() + tempStr.size(), temp);
				if (result.ec != errc{}) {
					retVal = defaultVal; // conversion failed
				}
				temp = temp > MAX ? MAX : temp;
				temp = temp < MIN ? MIN : temp;
				temp = temp >= minVal ? temp : defaultVal;
				temp = temp <= maxVal ? temp : defaultVal;
				retVal = static_cast<T>(temp);
			}
			else {
				uint64_t temp = 0;
				result = from_chars(tempStr.data(), tempStr.data() + tempStr.size(), temp);
				if (result.ec != errc{}) {
					retVal = defaultVal; // conversion failed
				}
				temp = temp > MAX ? MAX : temp;
				temp = temp < MIN ? MIN : temp;
				temp = temp >= minVal ? temp : defaultVal;
				temp = temp <= maxVal ? temp : defaultVal;
				retVal = static_cast<T>(temp);
			}
		}
		else {
			retVal = defaultVal;
		}
		return retVal;
	}

	bool jsonToBool(const json &jsonData, string_view jsonKeyVal, const bool defaultVal);
	string jsonToString(const json &jsonData, string_view jsonKeyVal, const string &defaultVal);
	Config loadConfig();
}
