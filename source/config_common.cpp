#include <fstream>
#include <iostream>
#include "config.h"
#include "config_common.h"
#include "utils.h"
#include "win_utils.h"


namespace config {
	using namespace std;
	namespace fs = std::filesystem;
	using json = nlohmann::ordered_json;

	bool jsonToBool(json::const_reference element, const bool defaultVal) {
		bool retVal;
		if (element.is_boolean()) {
			element.get_to(retVal);
		}
		else if (element.is_string()) {
			string temp;
			element.get_to(temp);
			temp = utils::trim(temp);
			temp = utils::lowercase(temp);
			if (temp == "true" || temp == "yes") {
				retVal = true;
			}
			else if (temp == "false" || temp == "no") {
				retVal = false;
			}
			else {
				retVal = defaultVal;
			}
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
	bool writeJson(ostream &os, const json &j) {
		const auto old_width = os.width();
		// dump() produces warning C28020 and doesn't seem fixable? Probably a false positive anyways.
		// So suppress warning for 1 line
		// ref: https://stackoverflow.com/a/25447795
#pragma warning(suppress: 28020)
		os << setw(4) << j << endl << setw(old_width);
		os.flush();
		return !os.fail();
	}

	//
	//
	//

	void saveConfig(const fs::path configPath, const Config newConfig) {
		if (fs::exists(configPath)) {
			const fs::path defaultBakPath(fmt::format("{:s}{:s}", configPath.string(), ".bak"));
			fs::path bakPath(defaultBakPath);
			int bakCounter = 1;
			while (fs::exists(bakPath)) {
				bakPath = fs::path(fmt::format("{:s}{:d}", defaultBakPath.string(), bakCounter++));
			}
			fs::rename(configPath, bakPath);
		}
		ofstream ofs = ofstream(configPath, ios::out | ios::binary | ios::trunc);
		writeJson(ofs, newConfig);
	}

	Config loadConfig() {
		fs::path configPath(fmt::format("{:s}{:s}", rsc_Name, ".json"));
		if (!fs::exists(configPath)) {
			const fs::path asiDir = winutils::GetASIPath().parent_path();
			configPath = fs::path(asiDir / configPath.filename());
		}

		const Config defaults = Config();
		try {
			json jj = winutils::getOptionDescriptions(); // for testing purposes, doesn't do anything useful yet

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
		catch (const json::exception &err) {
			//throw err;
			cerr << err.what();
			// JSON parse failed, so replace bad config file with defaults
			saveConfig(configPath, defaults);
			return defaults;
		}
		catch (const exception &err) {
			//throw err;
			cerr << err.what();
			return defaults;
		}
	}
}
