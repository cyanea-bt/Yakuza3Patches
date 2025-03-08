#include "config.h"
#include "config_common.h"


namespace config {
	using namespace std;
	using json = nlohmann::ordered_json;

	void to_json(json &j, const Config &e) {
		j = json();
		j["Version"] = e.Version;
		j["EnablePatch"] = e.EnablePatch;
		j["IgnoreGameCheck"] = e.IgnoreGameCheck;
		j["ThrowEnemy"] = e.ThrowEnemy;
		j["EnemyThrowResIncrease"] = e.EnemyThrowResIncrease;
		j["EscapeEnemyGrab"] = e.EscapeEnemyGrab;
		j["ChargeFeelTheHeat"] = e.ChargeFeelTheHeat;
	}

	void from_json(const json &j, Config &e) {
		const Config defaults = Config();
		e.Version = jsonToNumber<uint32_t>(j.at("Version"), 1, defaults.Version);
		e.EnablePatch = jsonToBool(j.at("EnablePatch"), defaults.EnablePatch);
		e.IgnoreGameCheck = jsonToBool(j.at("IgnoreGameCheck"), defaults.IgnoreGameCheck);
		e.ThrowEnemy = jsonToNumber<uint8_t>(j.at("ThrowEnemy"), 1, defaults.ThrowEnemy);
		e.EnemyThrowResIncrease = jsonToNumber<uint8_t>(j.at("EnemyThrowResIncrease"), 0, defaults.EnemyThrowResIncrease);
		e.EscapeEnemyGrab = jsonToNumber<uint8_t>(j.at("EscapeEnemyGrab"), 1, defaults.EscapeEnemyGrab);
		e.ChargeFeelTheHeat = jsonToNumber<uint8_t>(j.at("ChargeFeelTheHeat"), 1, defaults.ChargeFeelTheHeat);
		if (json test = e; test != j) {
			// Check if the converted config values differ from those in the JSON file.
			// If input JSON contained invalid values, we should overwrite the config file.
			e.SaveRequired = true;
		}
	}

	Config GetConfig() {
		return loadConfig();
	}
}
