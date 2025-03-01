#include <format>
#include <string>


namespace utils {
	std::string UTCString();
	std::string UTCString_ms();
	std::string TzString();
	std::string TzString_ms();
	void Log(std::string_view msg, const int channel = -1);
	void Log(std::string_view msg, const bool close, const int channel = -1);

	std::string replaceAll(const std::string &str, const std::string &from, const std::string &to);
	void replaceAllByRef(std::string &str, const std::string &from, const std::string &to);
	std::string_view ltrim(std::string_view str);
	std::string_view rtrim(std::string_view str);
	std::string_view trim(std::string_view str);
	std::string lowercase(std::string_view str);
	std::string uppercase(std::string_view str);

	// Can't seem to get the correct address of a function at runtime without this
	// ref: Utils/Trampoline.h
	template<typename Func>
	void *GetFuncAddr(Func func)
	{
		void *addr;
		memcpy(&addr, std::addressof(func), sizeof(addr));
		return addr;
	}
}
