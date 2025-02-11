#include <string>


namespace config {
	struct Config {
		std::string path;
		bool Enable = true;
		bool Force = false;
	};

	Config loadConfig();
}
