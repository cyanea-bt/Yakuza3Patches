#include <string>


namespace config {
	struct Config {
		std::string path;
		uint32_t Version = 1;
		bool Enable = true;
		bool Force = false;
	};

	Config loadConfig();
}
