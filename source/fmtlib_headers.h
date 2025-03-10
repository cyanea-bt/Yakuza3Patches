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
#define FMT_HEADER_ONLY
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <fmt/compile.h>
#include <fmt/ranges.h>
#include <fmt/std.h>
#include <fmt/os.h>
#ifdef __INTELLISENSE__
#pragma diag_default 1574, 2500
#endif
#pragma warning(pop)
