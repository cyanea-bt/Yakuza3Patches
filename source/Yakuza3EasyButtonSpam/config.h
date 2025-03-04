#include <nlohmann/json.hpp>


namespace config {
	using namespace std;
	using json = nlohmann::ordered_json;

	struct Config {
		string path;
		bool SaveRequired = false;
		uint32_t Version = 1;
		bool EnablePatch = true;
		bool IgnoreGameCheck = false;
		uint8_t ThrowEnemy = 2; // valid range: 1-255
		uint8_t EnemyThrowResIncrease = 1; // valid range: 0-255
		uint8_t EscapeEnemyGrab = 2; // valid range: 1-255
		uint8_t ChargeFeelTheHeat = 2; // valid range: 1-255
	};

	void to_json(json &j, const Config &e);
	void from_json(const json &j, Config &e);
	Config GetConfig();
}
