#include <filesystem>
#include "nlohmann/json.hpp"
#include "config.h"


namespace config {
	using namespace std;
	namespace fs = std::filesystem;
	using json = nlohmann::json;

	string loadConfig() {
		json j_string = "this is a string";
		string test;
		j_string.get_to(test);
		return test;
	}
}
