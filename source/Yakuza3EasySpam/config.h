#include <string>


namespace config {
	struct Config {
		std::string path;
		bool SaveRequired = false;
		uint32_t Version = 1;
		bool EnablePatch = true;
		bool ForcePatch = false;
		uint8_t EnemyThrowResDiv = 2;
		uint8_t EnemyThrowResInc = 1;
		uint8_t EnemyHoldPowerSub = 2;
		uint8_t FeelTheHeatChargeMulti = 2;
	};

	Config GetConfig();
}
