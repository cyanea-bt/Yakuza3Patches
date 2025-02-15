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
		j["EnemyThrowResDiv"] = e.EnemyThrowResDiv;
		j["EnemyThrowResInc"] = e.EnemyThrowResInc;
		j["EnemyHoldPowerSub"] = e.EnemyHoldPowerSub;
		j["FeelTheHeatChargeMulti"] = e.FeelTheHeatChargeMulti;
	}

	void from_json(const json &j, Config &e) {
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

	Config GetConfig() {
		if (!s_ConfigLoaded) {
			s_Config = loadConfig();
			s_ConfigLoaded = true;
		}
		return s_Config;
	}
}
