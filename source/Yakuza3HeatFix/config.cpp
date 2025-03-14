#include "config.h"
#include "config_common.h"


namespace config {
	using json = nlohmann::ordered_json;

	void to_json(json &j, const Config &e) {
		j = json();
		j["Version"] = e.Version;
		j["EnablePatch"] = e.EnablePatch;
		j["IgnoreGameCheck"] = e.IgnoreGameCheck;
		j["ShowHeatValues"] = e.ShowHeatValues;
		j["UseOldPatch"] = e.UseOldPatch;
		j["DrainTimeMulti"] = e.DrainTimeMulti;
		j["FixHeatDrain"] = e.FixHeatDrain;
		j["FixHeatGain"] = e.FixHeatGain;
		j["DisableHeatDrain"] = e.DisableHeatDrain;
		j["InfiniteHeat"] = e.InfiniteHeat;
		j["ZeroHeat"] = e.ZeroHeat;
	}

	void from_json(const json &j, Config &e) {
		const Config defaults = Config();
		e.Version = jsonToNumber<uint32_t>(j, "Version", 1, 1, defaults.Version);
		e.EnablePatch = jsonToBool(j, "EnablePatch", defaults.EnablePatch);
		e.IgnoreGameCheck = jsonToBool(j, "IgnoreGameCheck", defaults.IgnoreGameCheck);
		e.ShowHeatValues = jsonToBool(j, "ShowHeatValues", defaults.ShowHeatValues);
		e.UseOldPatch = jsonToBool(j, "UseOldPatch", defaults.UseOldPatch);
		e.DrainTimeMulti = jsonToNumber<uint8_t>(j, "DrainTimeMulti", 1, 200, defaults.DrainTimeMulti);
		e.FixHeatDrain = jsonToBool(j, "FixHeatDrain", defaults.FixHeatDrain);
		e.FixHeatGain = jsonToBool(j, "FixHeatGain", defaults.FixHeatGain);
		e.DisableHeatDrain = jsonToBool(j, "DisableHeatDrain", defaults.DisableHeatDrain);
		e.InfiniteHeat = jsonToBool(j, "InfiniteHeat", defaults.InfiniteHeat);
		e.ZeroHeat = jsonToBool(j, "ZeroHeat", defaults.ZeroHeat);
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
