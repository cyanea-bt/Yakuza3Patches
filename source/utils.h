#include <string>


namespace utils {
	std::string UTCString();
	std::string UTCString_ms();
	std::string TzString();
	std::string TzString_ms();

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
