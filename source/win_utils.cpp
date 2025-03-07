#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#define WINVER 0x0601
#define _WIN32_WINNT 0x0601

#include <windows.h>
#include "win_utils.h"
#include "resources/resource.h"


namespace winutils {
	using namespace std;
	namespace fs = std::filesystem;

	static HMODULE get_module_handle(void *address) {
		HMODULE hm = NULL;
		DWORD dwFlags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;

		if (!GetModuleHandleExW(dwFlags, static_cast<LPCWSTR>(address), &hm)) {
			ostringstream oss;
			oss << "GetModuleHandleExW returned: " << GetLastError();
			throw runtime_error(oss.str());
		}
		return hm;
	}

	static fs::path get_module_path(void *address) {
		WCHAR path[MAX_PATH];
		HMODULE hm = get_module_handle(address);
		GetModuleFileNameW(hm, path, MAX_PATH);
		const wstring p(path);
		return fs::path(p);
	}

	static HMODULE GetASIHandle() {
		return get_module_handle(get_module_handle);
	}

	// Get real path to .asi file, since the working dir will be the dir of the .exe that loaded it.
	// And that won't match the .asi file's path if it is loaded from inside the "mods" subdirectory.
	// ref: https://gist.github.com/pwm1234/05280cf2e462853e183d
	//      https://stackoverflow.com/questions/6924195/get-dll-path-at-runtime
	fs::path GetASIPath() {
		return get_module_path(get_module_path);
	}

	void showErrorMessage(string_view msg) {
		MessageBoxA(NULL, msg.data(), "ERROR", MB_OK | MB_ICONERROR | MB_TASKMODAL);
	}


	// Access embedded (resource) text file
	// ref: https://mklimenko.github.io/english/2018/06/23/embed-resources-msvc/
	//      https://learn.microsoft.com/en-us/answers/questions/1652442/accessing-resources-in-the-project-as-a-file-or-bi
	class Resource {
	public:
		struct Parameters {
			std::size_t size_bytes = 0;
			void *ptr = nullptr;
		};

	private:
		HRSRC hResource = nullptr;
		HGLOBAL hMemory = nullptr;

		Parameters p;

	public:
		Resource() = default;
		Resource(int resource_id, const std::wstring &resource_class) {
			HMODULE hm = GetASIHandle();
			hResource = FindResourceW(hm, MAKEINTRESOURCEW(resource_id), resource_class.data());
			if (!hResource) {
				throw runtime_error("Failed to find resource!");
			}
			hMemory = LoadResource(hm, hResource);
			if (!hMemory) {
				throw runtime_error("Failed to load resource!");
			}
			p.size_bytes = SizeofResource(hm, hResource);
			if (p.size_bytes == 0) {
				throw runtime_error("Resource size is zero!");
			}
			p.ptr = LockResource(hMemory);
			if (!p.ptr) {
				throw runtime_error("Failed to lock resource!");
			}
		}

		string_view GetResourceString() const {
			std::string_view dst;
			if (p.ptr != nullptr) {
				dst = std::string_view(static_cast<char *>(p.ptr), p.size_bytes);
			}
			return dst;
		}
	};


	static Resource optionDescriptions;

	string_view getOptionDescriptions() {
		if (optionDescriptions.GetResourceString().empty()) {
			optionDescriptions = Resource(IDR_TEXT1, L"TEXT");
		}
		return optionDescriptions.GetResourceString();
	}
}
