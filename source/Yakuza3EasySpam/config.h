#include "nlohmann/json.hpp"


namespace config {
	using namespace std;
	using json = nlohmann::ordered_json;

	struct Config {
		string path;
		bool SaveRequired = false;
		uint32_t Version = 1;
		bool EnablePatch = true;
		bool ForcePatch = false;
		uint8_t EnemyThrowResDiv = 2; // valid range: 1-255
		uint8_t EnemyThrowResInc = 1; // valid range: 0-255
		uint8_t EnemyHoldPowerSub = 2; // valid range: 1-255
		uint8_t FeelTheHeatChargeMulti = 2; // valid range: 1-255
	};

	void to_json(json &j, const Config &e);
	void from_json(const json &j, Config &e);
	Config GetConfig();
}
