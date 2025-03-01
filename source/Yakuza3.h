#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#define WINVER 0x0601
#define _WIN32_WINNT 0x0601

#include "ModUtils/MemoryMgr.h"
#include "ModUtils/Trampoline.h"
#include "ModUtils/Patterns.h"
#include "config.h"

#if _DEBUG
inline constexpr bool isDEBUG = true;
#else
inline constexpr bool isDEBUG = false;
#endif // _DEBUG
extern config::Config CONFIG;


namespace Yakuza3 {
	bool Init();
}
