#include <filesystem>
#include <fstream>
#include <iostream>
#include <ostream>
#include "config.h"
#include "config_common.h"
#include "utils.h"
#include "win_utils.h"
#include "fmtlib_headers.h"
#include "spdlog_headers.h"


namespace config {
	using std::ios;
	using std::ifstream;
	using std::ofstream;
	namespace fs = std::filesystem;
	using json = nlohmann::ordered_json;

	bool jsonToBool(const json &jsonData, string_view jsonKeyVal, const bool defaultVal) {
		if (!jsonData.contains(jsonKeyVal)) {
			return defaultVal;
		}
		const auto element = jsonData[jsonKeyVal];
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

	string jsonToString(const json &jsonData, string_view jsonKeyVal, const string &defaultVal) {
		if (!jsonData.contains(jsonKeyVal)) {
			return defaultVal;
		}
		const auto element = jsonData[jsonKeyVal];
		string retVal;
		if (element.is_string()) {
			element.get_to(retVal);
		}
		else {
			retVal = defaultVal;
		}
		return retVal;
	}

	//
	//
	//

	// write prettified JSON to ostream
	static bool writeJson(std::ostream &os, const json &j) {
		const auto old_width = os.width();
		// dump() produces warning C28020 and doesn't seem fixable? Probably a false positive anyways.
		// So suppress warning for 1 line
		// ref: https://stackoverflow.com/a/25447795
#pragma warning(suppress: 28020)
		os << std::setw(4) << j << std::endl << std::setw(old_width);
		os.flush();
		return !os.fail();
	}

	static void exportDescriptions(const fs::path descPath) {
		json j = json::parse(winutils::GetOptionDescriptions());
		ofstream ofs = ofstream(descPath, ios::out | ios::binary | ios::trunc);
		
		string info = j["info"];
		utils::replaceAllByRef(info, "{{CONFIG_FILE}}", fmt::format("\"{:s}{:s}\"", rsc_Name, ".json"));
		ofs << fmt::format("{:s}\n\n\n\n", info);

		for (const auto &element : j["options"]) {
			const string name = element["Name"];
			const string desc = element["Description"];
			ofs << fmt::format(">>> Option: {:s}\n", name);
			ofs << fmt::format("{:s}\n", desc);

			if (element["Default"].is_string()) {
				const string def = element["Default"];
				ofs << fmt::format("Default value:    {:s}\n", def);
			}
			else if (element["Default"].is_boolean()) {
				const bool def = element["Default"];
				ofs << fmt::format("Default value:    {}\n", def);
				ofs << fmt::format("Supported values: {} / {}\n", true, false);
			}
			else if (element["Default"].is_number_unsigned()) {
				const uint64_t def = element["Default"];
				const uint64_t min = element["Min"];
				const uint64_t max = element["Max"];
				ofs << fmt::format("Default value:    {:d}\n", def);
				ofs << fmt::format("Minimum value:    {:d}\n", min);
				ofs << fmt::format("Maximum value:    {:d}\n", max);
			}
			else if (element["Default"].is_number_integer()) {
				const int64_t def = element["Default"];
				const int64_t min = element["Min"];
				const int64_t max = element["Max"];
				ofs << fmt::format("Default value:    {:d}\n", def);
				ofs << fmt::format("Minimum value:    {:d}\n", min);
				ofs << fmt::format("Maximum value:    {:d}\n", max);
			}
			else if (element["Default"].is_number_float()) {
				const float def = element["Default"];
				const float min = element["Min"];
				const float max = element["Max"];
				ofs << fmt::format("Default value:    {:.2f}\n", def);
				ofs << fmt::format("Minimum value:    {:.2f}\n", min);
				ofs << fmt::format("Maximum value:    {:.2f}\n", max);
			}
			ofs << std::endl;
		}
	}

	static void saveConfig(const fs::path configPath, const Config newConfig) {
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
		const fs::path descPath(configPath.parent_path() / fmt::format("{:s}{:s}", rsc_Name, "_Info.txt"));
		exportDescriptions(descPath);
	}


	Config loadConfig() {
		// Try to find config file in current working directory
		fs::path configPath(fmt::format("{:s}{:s}", rsc_Name, ".json"));
		if (!fs::exists(configPath)) {
			// Otherwise try to find config file in mod directory
			const fs::path asiDir = winutils::GetASIPath().parent_path();
			configPath = fs::path(asiDir / configPath.filename());
		}

		const Config defaults = Config();
		try {
			Config loaded;
			if (!fs::exists(configPath)) {
				// no config exists, so create default config next to .asi file
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
					// replace outdated/invalid config file with new defaults
					saveConfig(configPath, defaults);
					loaded = defaults;
				}
			}
			loaded.path = configPath.string();
			return loaded;
		}
		catch (const json::exception &err) {
			spdlog::error(err.what());
			// JSON parse failed, so replace bad config file with defaults
			try {
				saveConfig(configPath, defaults);
			}
			catch (const std::exception &err) {
				spdlog::error(err.what());
			}
			return defaults;
		}
		catch (const std::exception &err) {
			spdlog::error(err.what());
			return defaults;
		}
	}
}
