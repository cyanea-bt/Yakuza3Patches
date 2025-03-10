#pragma once
#include <nlohmann/json.hpp>


namespace config {
	using std::string;
	using json = nlohmann::ordered_json;

	struct Config {
		string path;
		bool SaveRequired = false;
		uint32_t Version = 1;
		bool EnablePatch = true;
		bool IgnoreGameCheck = false;
		bool ShowHeatValues = false;
		bool UseOldPatch = false;
		uint8_t DrainTimeMulti = 2; // valid range: 1-255
		bool FixHeatDrain = true;
		bool FixHeatGain = true;
		bool DisableHeatDrain = false;
		bool InfiniteHeat = false;
		bool ZeroHeat = false;
	};

	void to_json(json &j, const Config &e);
	void from_json(const json &j, Config &e);
	Config GetConfig();
}
