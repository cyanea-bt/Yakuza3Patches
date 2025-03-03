#include "utils.h"
#include "Yakuza3.h"


namespace EnemyAIFix {
	using namespace std;
	static uint64_t dbg_Counter1 = 0, dbg_Counter2 = 0, dbg_Counter3 = 0, dbg_Counter4 = 0, dbg_Counter5 = 0;
}


void OnInitializeHook()
{
	using namespace std;
	using namespace Memory;
	using namespace hook;

	unique_ptr<ScopedUnprotect::Unprotect> Protect = ScopedUnprotect::UnprotectSectionOrFullModule(GetModuleHandle(nullptr), ".text");

	// Check if patch should be disabled
	if (!Yakuza3::Init()) {
		return;
	}

	/*
	* Hook into the functions that control enemy AI behavior to try and make
	* the enemies react slower / block less of the player's attacks.
	*
	* SHA-1 checksums for compatible binaries:
	* 20d9614f41dc675848be46920974795481bdbd3b Yakuza3.exe (Steam)
	* 2a55a4b13674d4e62cda2ff86bc365d41b645a92 Yakuza3.exe (Steam without SteamStub)
	* 6c688b51650fa2e9be39e1d934e872602ee54799 Yakuza3.exe (GOG)
	*/
	{
		using namespace EnemyAIFix;

		if (isDEBUG) {
			// Open debug logfile streams (not necessary but will save some time on the first real log message)
			utils::Log("", 1);
			utils::Log("", 2);
			utils::Log("", 3);
			utils::Log("", 4);
			utils::Log("", 5);
		}


	}

	// log current time to file to get some feedback once hook is done
	utils::Log("Hook done!");
	utils::Log(fmt::format("Config path: \"{:s}\"", CONFIG.path));
	utils::Log(fmt::format("Local: {:s}", utils::TzString()));
	utils::Log(fmt::format("UTC:   {:s}", utils::UTCString()), true);
}
