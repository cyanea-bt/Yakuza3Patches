#include <string>


namespace config {
	struct Config {
		std::string path;
		bool SaveRequired = false;
		uint32_t Version = 1;
		bool EnablePatch = true;
		bool ForcePatch = false;
	};

	Config GetConfig();
}
