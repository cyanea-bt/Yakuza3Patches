#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#define WINVER 0x0601
#define _WIN32_WINNT 0x0601

#include <windows.h>
#include "winutils.h"


namespace winutils {
	using namespace std;
	namespace fs = std::filesystem;

	static fs::path get_module_path(void *address)
	{
		WCHAR path[MAX_PATH];
		HMODULE hm = NULL;

		if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
			GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			(LPCWSTR)address,
			&hm))
		{
			ostringstream oss;
			oss << "GetModuleHandle returned " << GetLastError();
			throw runtime_error(oss.str());
		}
		GetModuleFileNameW(hm, path, MAX_PATH);

		wstring p(path);
		return fs::path(p);
	}

	// Get real path to .asi file, since the working dir will be the dir of the .exe that loaded it.
	// And that won't match the .asi file's path if it is loaded from inside the "mods" subdirectory.
	// ref: https://gist.github.com/pwm1234/05280cf2e462853e183d
	//      https://stackoverflow.com/questions/6924195/get-dll-path-at-runtime
	fs::path GetASIPath() {
		return get_module_path(get_module_path);
	}
}
