#pragma warning(push) // fmtlib/spdlog warnings
#pragma warning(disable : 6294)
#pragma warning(disable : 26495)
#pragma warning(disable : 26498)
#pragma warning(disable : 26451)
#pragma warning(disable : 26812)
#pragma warning(disable : 26439)
#pragma warning(disable : 28251)
#ifdef __INTELLISENSE__
#pragma diag_suppress 1574, 2500 // nonsense Intellisense errors for fmtlib
#endif
#define FMT_HEADER_ONLY
#define SPDLOG_HEADER_ONLY
#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <fmt/compile.h>
#include <fmt/ranges.h>
#include <fmt/std.h>
#include <fmt/os.h>
#include <spdlog/spdlog.h>
#include <spdlog/counter_flag_formatter.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/basic_lazy_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#ifdef __INTELLISENSE__
#pragma diag_default 1574, 2500
#endif
#pragma warning(pop)

// I kinda hate this but I couldn't find a better solution
// ref: https://stackoverflow.com/a/73512139
#define cformat(fmt, ...) format(FMT_COMPILE(fmt), ##__VA_ARGS__)


namespace utils {
	std::string UTCString();
	std::string UTCString_ms();
	std::string TzString();
	std::string TzString_ms();
	std::string UTCFilename();
	std::string UTCFilename_ms();
	std::string TzFilename();
	std::string TzFilename_ms();
	void Log(std::string_view msg, const int channel = -1, std::string_view loggerName = {});
	void Log(std::string_view msg, const bool close, const int channel = -1, std::string_view loggerName = {});

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

	// Wrapper around fmt::format()
	// ref: https://github.com/fmtlib/fmt/issues/2391#issuecomment-869043202
	template<typename... T>
	void Log(const int channel, fmt::format_string<T...> fmt, T&&... args) {
		const auto str = fmt::format(fmt, std::forward<T>(args)...);
		Log(str, channel);
	}
}
