#pragma once
#include <filesystem>
#include <string>


namespace winutils {
	using std::string;
	using std::wstring;
	using std::string_view;
	namespace fs = std::filesystem;

	fs::path GetASIPath();
	void ShowErrorMessage(string_view msg);
	string_view GetOptionDescriptions();
	string wstr_to_utf8(const wstring &wstr);
	wstring utf8_to_wstr(const string &utf8str);
}
