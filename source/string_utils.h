#pragma once
#include <string>


namespace utils {
	using std::string;
	using std::string_view;

	string UTCString();
	string UTCString_ms();
	string TzString();
	string TzString_ms();
	string UTCFilename();
	string UTCFilename_ms();
	string TzFilename();
	string TzFilename_ms();

	string replaceAll(const string &str, const string &from, const string &to);
	void replaceAllByRef(string &str, const string &from, const string &to);
	string_view ltrim(string_view str);
	string_view rtrim(string_view str);
	string_view trim(string_view str);
	string lowercase(string_view str);
	string uppercase(string_view str);
}
