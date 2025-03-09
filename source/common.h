#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#define WINVER 0x0601
#define _WIN32_WINNT 0x0601

#if _DEBUG
inline constexpr bool isDEBUG = true;
#else
inline constexpr bool isDEBUG = false;
#endif // _DEBUG

// Avoids crashes if no debugger is attached.
#define DbgBreak() { if (IsDebuggerPresent()) { DebugBreak(); } }
