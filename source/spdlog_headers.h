#pragma once
#pragma warning(push) // fmtlib/spdlog Intellisense warnings
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
#define SPDLOG_HEADER_ONLY
#define SPDLOG_FMT_EXTERNAL_HO
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
