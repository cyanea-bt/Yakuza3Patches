#include "config.h"
#include "config_common.h"


namespace config {
	using namespace std;
	using json = nlohmann::ordered_json;

	void to_json(json &j, const Config &e) {
		j = json();
		j["Version"] = e.Version;
		j["EnablePatch"] = e.EnablePatch;
		j["ForcePatch"] = e.ForcePatch;
		j["UseOldPatch"] = e.UseOldPatch;
		j["DrainTimeMulti"] = e.DrainTimeMulti;
		j["DisableHeatDrain"] = e.DisableHeatDrain;
	}

	void from_json(const json &j, Config &e) {
		const Config defaults = Config();
		e.Version = jsonToNumber<uint32_t>(j.at("Version"), 1, defaults.Version);
		e.EnablePatch = jsonToBool(j.at("EnablePatch"), defaults.EnablePatch);
		e.ForcePatch = jsonToBool(j.at("ForcePatch"), defaults.ForcePatch);
		e.UseOldPatch = jsonToBool(j.at("UseOldPatch"), defaults.UseOldPatch);
		e.DrainTimeMulti = jsonToNumber<uint8_t>(j.at("DrainTimeMulti"), 1, defaults.DrainTimeMulti);
		e.DisableHeatDrain = jsonToBool(j.at("DisableHeatDrain"), defaults.DisableHeatDrain);
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

	Config GetConfig() {
		if (!s_ConfigLoaded) {
			s_Config = loadConfig();
			s_ConfigLoaded = true;
		}
		return s_Config;
	}
}
