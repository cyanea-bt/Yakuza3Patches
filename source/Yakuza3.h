#pragma once
#include "common.h"
#include "ModUtils/MemoryMgr.h"
#include "ModUtils/Trampoline.h"
#include "ModUtils/Patterns.h"
#include "config.h"


inline config::Config CONFIG;

namespace Yakuza3 {
	bool Init();
	void Finalize(std::string_view finalMsg);
}
