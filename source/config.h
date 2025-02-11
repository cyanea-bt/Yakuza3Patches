#include <string>


namespace config {
	struct Config {
		std::string path;
		bool Enable = true;
	};

	Config loadConfig();
}
