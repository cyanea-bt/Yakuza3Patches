#pragma once
#include <string>


namespace utils {
	std::string UTCString();
	std::string UTCString_ms();
	std::string TzString();
	std::string TzString_ms();
	std::string UTCFilename();
	std::string UTCFilename_ms();
	std::string TzFilename();
	std::string TzFilename_ms();

	std::string replaceAll(const std::string &str, const std::string &from, const std::string &to);
	void replaceAllByRef(std::string &str, const std::string &from, const std::string &to);
	std::string_view ltrim(std::string_view str);
	std::string_view rtrim(std::string_view str);
	std::string_view trim(std::string_view str);
	std::string lowercase(std::string_view str);
	std::string uppercase(std::string_view str);
}
