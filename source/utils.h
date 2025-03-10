#pragma once
#include "fmtlib_headers.h"


namespace utils {
	using std::string;
	using std::string_view;

	void Log(string_view msg, const int channel = -1, string_view loggerName = {});
	void Log(string_view msg, const bool close, const int channel = -1, string_view loggerName = {});

	// Can't seem to get the correct address of a function at runtime without this
	// ref: Utils/Trampoline.h
	template<typename Func>
	void *GetFuncAddr(Func func)
	{
		void *addr;
		memcpy(&addr, std::addressof(func), sizeof(addr));
		return addr;
	}

	// Wrapper around fmt::format()
	// ref: https://github.com/fmtlib/fmt/issues/2391#issuecomment-869043202
	template<typename... T>
	void Log(const int channel, fmt::format_string<T...> fmt, T&&... args) {
		const auto str = fmt::format(fmt, std::forward<T>(args)...);
		Log(str, channel);
	}
}
