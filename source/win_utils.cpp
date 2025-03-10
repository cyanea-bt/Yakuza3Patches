#include "common.h"
#include <windows.h>
#include "resources/resource.h"
#include "win_utils.h"
#include "fmtlib_headers.h"
#include "spdlog_headers.h"


namespace winutils {
	// Log error message from Win32 GetLastError() to default spdlog logger
	// ref: https://learn.microsoft.com/en-us/windows/win32/debug/retrieving-the-last-error-code
	//      https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-formatmessage
	//      https://learn.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
	static void LogLastError(string_view fnName) {
		LPVOID lpMsgBuf = NULL;
		const DWORD lastError = GetLastError();
		const DWORD dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
		const DWORD langID = static_cast<DWORD>(LocaleNameToLCID(L"en-US", 0));
		const DWORD formatResult = FormatMessageW(dwFlags, NULL, lastError, langID, reinterpret_cast<LPWSTR>(&lpMsgBuf), 0, NULL);
		spdlog::error("{0:s} returned code: {1:#x} ({1:d})", fnName, lastError);
		if (formatResult != 0) {
			const wstring msg(static_cast<LPWSTR>(lpMsgBuf));
			const string utf8str = wstr_to_utf8(msg);
			if (isDEBUG) {
				// validate converted string
				const wstring wstr = utf8_to_wstr(utf8str);
				if (wstr != msg) {
					DbgBreak();
					spdlog::error("LogLastError: FormatMessageW string validation failed!");
				}
			}
			spdlog::error("ErrorMsg: {:s}", utf8str);
		}
	}

	// Convert wide character string to UTF8 string using Win32 API
	// ref: https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte
	string wstr_to_utf8(const wstring &wstr) {
		if (wstr.empty()) {
			return {};
		}
		const DWORD dwFlags = WC_ERR_INVALID_CHARS;
		const int sourceLen = static_cast<int>(wstr.size());
		// returns buffer size in bytes
		const int bufSize = WideCharToMultiByte(CP_UTF8, dwFlags, wstr.data(), sourceLen, NULL, 0, NULL, NULL);
		static_assert(sizeof(char) == sizeof(uint8_t), "Size of char and uint8_t doesn't match!");
		if (bufSize == 0) {
			LogLastError("WideCharToMultiByte");
			return {};
		}
		string result(bufSize, '\0');
		const int bytesWritten = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), sourceLen, result.data(), bufSize, NULL, NULL);
		if (bytesWritten != bufSize) {
			LogLastError("WideCharToMultiByte");
			return {};
		}
		return result;
	}

	// Convert UTF8 string to wide character string using Win32 API
	// ref: https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar
	wstring utf8_to_wstr(const string &utf8str) {
		if (utf8str.empty()) {
			return {};
		}
		const DWORD dwFlags = MB_ERR_INVALID_CHARS;
		const int sourceLen = static_cast<int>(utf8str.size());
		// returns buffer size in number of characters
		const int bufSize = MultiByteToWideChar(CP_UTF8, dwFlags, utf8str.data(), sourceLen, NULL, 0);
		if (bufSize == 0) {
			LogLastError("MultiByteToWideChar");
			return {};
		}
		wstring result(bufSize, L'\0');
		const int charsWritten = MultiByteToWideChar(CP_UTF8, 0, utf8str.data(), sourceLen, result.data(), bufSize);
		if (charsWritten != bufSize) {
			LogLastError("MultiByteToWideChar");
			return {};
		}
		return result;
	}

	static HMODULE GetModuleHandleByName(const string &moduleName) {
		HMODULE hm = NULL;
		const DWORD dwFlags = GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;
		const wstring wModuleName = utf8_to_wstr(moduleName);
		if (!GetModuleHandleExW(dwFlags, wModuleName.data(), &hm)) {
			LogLastError("GetModuleHandleExW");
		}
		return hm;
	}

	static HMODULE GetModuleHandleByAddress(const void *address) {
		HMODULE hm = NULL;
		const DWORD dwFlags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;
		if (!GetModuleHandleExW(dwFlags, static_cast<LPCWSTR>(address), &hm)) {
			LogLastError("GetModuleHandleExW");
		}
		return hm;
	}

	static fs::path GetModulePath(const HMODULE hm) {
		WCHAR path[MAX_PATH] = { 0 };
		const DWORD result = GetModuleFileNameW(hm, path, MAX_PATH);
		if (result == 0) {
			LogLastError("GetModuleFileNameW");
			return {};
		}
		const wstring p(path);
		return fs::path(p);
	}

	// Get handle to current module by calling GetModuleHandleExW with the address of a static/internal function
	// ref: https://gist.github.com/pwm1234/05280cf2e462853e183d
	//      https://stackoverflow.com/questions/6924195/get-dll-path-at-runtime
	static HMODULE GetASIHandle() {
		const HMODULE hm = GetModuleHandleByAddress(LogLastError);
		if (isDEBUG) {
			const string moduleName = fmt::format("{:s}{:s}", rsc_Name, rsc_Extension);
			const HMODULE hm2 = GetModuleHandleByName(moduleName);
			if (hm2 != hm) {
				DbgBreak();
				spdlog::error("GetASIHandle: Module handles don't match!");
			}
		}
		return hm;
	}

	// Get real path to .asi file, since the current working dir will be the directory of the .exe that loaded the .asi file.
	// And that directory/path won't match the .asi file's path if it is loaded from inside the "mods" subdirectory.
	fs::path GetASIPath() {
		return GetModulePath(GetASIHandle());
	}

	void ShowErrorMessage(string_view msg) {
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
		Resource(const int resource_id, const wstring &resource_class) {
			const HMODULE hm = GetASIHandle();
			hResource = FindResourceW(hm, MAKEINTRESOURCEW(resource_id), resource_class.data());
			if (!hResource) {
				throw std::runtime_error("Failed to find resource!");
			}
			hMemory = LoadResource(hm, hResource);
			if (!hMemory) {
				throw std::runtime_error("Failed to load resource!");
			}
			p.size_bytes = SizeofResource(hm, hResource);
			if (p.size_bytes == 0) {
				throw std::runtime_error("Resource size is zero!");
			}
			p.ptr = LockResource(hMemory);
			if (!p.ptr) {
				throw std::runtime_error("Failed to lock resource!");
			}
		}

		string_view GetResourceString() const {
			string_view dst;
			if (p.ptr != nullptr) {
				dst = string_view(static_cast<char *>(p.ptr), p.size_bytes);
			}
			return dst;
		}
	};


	static Resource optionDescriptions;

	string_view GetOptionDescriptions() {
		if (optionDescriptions.GetResourceString().empty()) {
			optionDescriptions = Resource(IDR_TEXT1, L"TEXT"); // TEXT = user-defined resource type
		}
		return optionDescriptions.GetResourceString();
	}
}
