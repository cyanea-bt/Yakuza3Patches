#include <string>


namespace config {
	struct Config {
		bool Enable = true;
	};

	Config loadConfig();
}
