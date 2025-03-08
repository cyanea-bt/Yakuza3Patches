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
	}

	void from_json(const json &j, Config &e) {
		const Config defaults = Config();
		e.Version = jsonToNumber<uint32_t>(j, "Version", 1, defaults.Version);
		e.EnablePatch = jsonToBool(j, "EnablePatch", defaults.EnablePatch);
		e.IgnoreGameCheck = jsonToBool(j, "IgnoreGameCheck", defaults.IgnoreGameCheck);
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
