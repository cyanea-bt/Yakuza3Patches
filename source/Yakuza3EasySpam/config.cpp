#include <iostream>
#include <filesystem>
#include <fstream>
#include <limits>
#include "nlohmann/json.hpp"
#include "config.h"
#include "win_utils.h"


namespace config {
	using namespace std;
	namespace fs = std::filesystem;
	using json = nlohmann::ordered_json;

	template<typename T>
	T jsonToNumber(json::const_reference element, const T minVal, const T defaultVal) {
		int64_t temp = 0;
		T retVal;
		if (element.is_number()) {
			element.get_to(temp);
			T MAX = numeric_limits<T>::max();
			T MIN = numeric_limits<T>::min();
			temp = temp > MAX ? MAX : temp;
			temp = temp < MIN ? MIN : temp;
			temp = temp >= minVal ? temp : defaultVal;
			retVal = static_cast<T>(temp);
		}
		else {
			retVal = defaultVal;
		}
		return retVal;
	}

	bool jsonToBool(json::const_reference element, const bool defaultVal) {
		bool retVal;
		if (element.is_boolean()) {
			element.get_to(retVal);
		}
		else {
			retVal = defaultVal;
		}
		return retVal;
	}

	string jsonToString(json::const_reference element, const string &defaultVal) {
		string retVal;
		if (element.is_string()) {
			element.get_to(retVal);
		}
		else {
			retVal = defaultVal;
		}
		return retVal;
	}

	// write prettified JSON to ostream
	static bool writeJson(ostream &os, const json &j) {
		const auto old_width = os.width();
		// dump() produces warning C28020 and doesn't seem fixable? Probably a false positive anyways.
		// So suppress warning for 1 line
		// ref: https://stackoverflow.com/a/25447795
#pragma warning(suppress: 28020)
		os << setw(4) << j << endl << setw(old_width);
		os.flush();
		return !os.fail();
	}

	static void to_json(json &j, const Config &e) {
		j = json();
		j["Version"] = e.Version;
		j["EnablePatch"] = e.EnablePatch;
		j["ForcePatch"] = e.ForcePatch;
		j["EnemyThrowResDiv"] = e.EnemyThrowResDiv;
		j["EnemyThrowResInc"] = e.EnemyThrowResInc;
		j["EnemyHoldPowerSub"] = e.EnemyHoldPowerSub;
		j["FeelTheHeatChargeMulti"] = e.FeelTheHeatChargeMulti;
	}

	static void from_json(const json &j, Config &e) {
		Config defaults = Config();
		e.Version = jsonToNumber<uint32_t>(j.at("Version"), 1, defaults.Version);
		e.EnablePatch = jsonToBool(j.at("EnablePatch"), defaults.EnablePatch);
		e.ForcePatch = jsonToBool(j.at("ForcePatch"), defaults.ForcePatch);
		e.EnemyThrowResDiv = jsonToNumber<uint8_t>(j.at("EnemyThrowResDiv"), 1, defaults.EnemyThrowResDiv);
		e.EnemyThrowResInc = jsonToNumber<uint8_t>(j.at("EnemyThrowResInc"), 0, defaults.EnemyThrowResInc);
		e.EnemyHoldPowerSub = jsonToNumber<uint8_t>(j.at("EnemyHoldPowerSub"), 1, defaults.EnemyHoldPowerSub);
		e.FeelTheHeatChargeMulti = jsonToNumber<uint8_t>(j.at("FeelTheHeatChargeMulti"), 1, defaults.FeelTheHeatChargeMulti);
		if (json test = e; test != j) {
			// Check if the converted config values differ from those in the JSON file.
			// If input JSON contained invalid values, we should overwrite the config file.
			e.SaveRequired = true;
		}
	}

	//
	//
	//

	static Config s_Config;
	static bool s_ConfigLoaded = false;

	static void saveConfig(const fs::path configPath, const Config newConfig) {
		if (fs::exists(configPath)) {
			const fs::path defaultBakPath(format("{:s}{:s}", configPath.string(), ".bak"));
			fs::path bakPath(defaultBakPath);
			int bakCounter = 1;
			while (fs::exists(bakPath)) {
				bakPath = fs::path(format("{:s}{:d}", defaultBakPath.string(), bakCounter++));
			}
			fs::rename(configPath, bakPath);
		}
		ofstream ofs = ofstream(configPath, ios::out | ios::binary | ios::trunc);
		writeJson(ofs, newConfig);
	}

	static Config loadConfig() {
		fs::path configPath(format("{:s}{:s}", rsc_Name, ".json"));
		if (!fs::exists(configPath)) {
			const fs::path asiDir = winutils::GetASIPath().parent_path();
			configPath = fs::path(asiDir / configPath.filename());
		}
		
		Config defaults = Config();
		try {
			Config loaded;
			if (!fs::exists(configPath)) {
				// create default config next to .asi file
				saveConfig(configPath, defaults);
				loaded = defaults;
			}
			else {
				// read existing config
				ifstream ifs = ifstream(configPath, ios::in | ios::binary);
				const json data = json::parse(ifs, nullptr, true, true); // ignore comments
				ifs.close();
				
				uint32_t version = 0;
				if (data.contains("Version")) {
					version = data["Version"];
				}
				if (version == defaults.Version) {
					// config version matches, should be able to parse
					loaded = data;
					if (loaded.SaveRequired) {
						saveConfig(configPath, loaded);
					}
				}
				else {
					// replace outdated config file with new defaults
					saveConfig(configPath, defaults);
					loaded = defaults;
				}
			}
			loaded.path = configPath.string();
			return loaded;
		}
		catch (const exception &err) {
			//throw err;
			cerr << err.what();
			return defaults;
		}
	}

	Config GetConfig() {
		if (!s_ConfigLoaded) {
			s_Config = loadConfig();
			s_ConfigLoaded = true;
		}
		return s_Config;
	}
}
