#include <windows.h>

#define HOOKED_FUNCTION GetCommandLineA
#define HOOKED_LIBRARY "KERNEL32.DLL"

#include "ModUtils/HookInit.hpp"
