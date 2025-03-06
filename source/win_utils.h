#include <filesystem>
#include <string>


namespace winutils {
	std::filesystem::path GetASIPath();
	void showErrorMessage(std::string_view msg);
}
