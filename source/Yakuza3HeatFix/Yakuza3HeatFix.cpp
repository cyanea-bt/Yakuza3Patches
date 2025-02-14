#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#define WINVER 0x0601
#define _WIN32_WINNT 0x0601

#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <windows.h>
#include <ShlObj.h>

#include "Utils/MemoryMgr.h"
#include "Utils/Trampoline.h"
#include "Utils/Patterns.h"
#include "config.h"

#if _DEBUG
static constexpr bool s_Debug = true;
#else
static constexpr bool s_Debug = false;
#endif // _DEBUG


namespace HeatFix {
	using namespace std;
	using namespace std::chrono;
	static const auto tz = current_zone();
	static ofstream ofs1, ofs2;
	static uint64_t dbg_Counter1 = 0, dbg_Counter2 = 0;
	static string dbg_msg;
	static bool logFailed = false;

	/*
	* Parameter types aren't correct but it shouldn't matter for our purposes.
	* param1 is probably a pointer to some kind of class instance
	* No clue what the other 3 are, but all 4 parameters seem to be 8 bytes wide.
	*/
	static void(*origHeatFunc)(uintptr_t param1, uintptr_t param2, uintptr_t param3, uintptr_t param4);
	static int counter = 0;

	static uint32_t(*IsPlayerInCombat)();
	static uintptr_t(*IsCombatInactive)();

	typedef uint32_t(*IsActorDeadType)(uintptr_t);
	static IsActorDeadType IsActorDead = nullptr;

	static uintptr_t globalPointer = 0;
	static uintptr_t pIsCombatFinished = 0;
	static uintptr_t isCombatFinished = 0;

	static uint32_t isCombatPausedByTutorial = 0;
	static uint32_t isCombatInTransition = 0; 

	static void PatchedHeatFunc(uintptr_t param1, uintptr_t param2, uintptr_t param3, uintptr_t param4) {
		// MOV  param_1,qword ptr [DAT_14122cde8]
		// CMP  dword ptr[param_1 + 0x8], EAX
		pIsCombatFinished = *(uintptr_t *)globalPointer;
		isCombatFinished = *(uintptr_t *)(pIsCombatFinished + 8);

		// MOV  RDX,qword ptr [param_1]
		// CALL qword ptr [RDX + 0x268]
		uintptr_t pIsActorDead = *(uintptr_t *)(param1);
		pIsActorDead = *(uintptr_t *)(pIsActorDead + 0x268);
		IsActorDead = (IsActorDeadType)pIsActorDead;

		// MOV  RBX,param_1
		// TEST dword ptr [RBX + 0x13d8],0x10000
		isCombatPausedByTutorial = *(uint32_t *)(param1 + 0x13d8);
		isCombatPausedByTutorial &= 0x10000; // Seems to be != 0 if combat is still active but is "paused" by a tutorial popup message

		// MOV  RBX,param_1
		// MOV  RCX,qword ptr [RBX + 0x13d0]
		// TEST dword ptr [RCX + 0x354],0x4000000
		uintptr_t pIsCombatInTransition = *(uintptr_t *)(param1 + 0x13d0);
		isCombatInTransition = *(uint32_t *)(pIsCombatInTransition + 0x354);
		isCombatInTransition &= 0x4000000; // Seems to be != 0 during combat intro or when fading to black after the player dies

		if (s_Debug) {
			if (!ofs1.is_open() && !logFailed) {
				ofs1 = ofstream("Yakuza3HeatFix_Debug1.txt", ios::out | ios::trunc | ios::binary);
			}
			if (!ofs2.is_open() && !logFailed) {
				ofs2 = ofstream("Yakuza3HeatFix_Debug2.txt", ios::out | ios::trunc | ios::binary);
			}
			if (!ofs1.is_open() || !ofs2.is_open()) {
				logFailed = true; // don't keep on trying to write logs if open failed (e.g. in case of missing write permissions)
			}
			else {
				const auto utcNow = system_clock::now();
				const auto tzNow = tz->to_local(utcNow);
				const string str_TzNow = format("{:%Y/%m/%d %H:%M:%S}", tzNow);
				dbg_msg = format("- TzNow: {:s} - IsPlayerInCombat: {:d} - IsCombatInactive: {:d} - isCombatPausedByTutorial: {:d} - IsActorDead: {:d} - isCombatInTransition: {:d} - isCombatFinished: {:d}", 
					str_TzNow, IsPlayerInCombat(), IsCombatInactive(), isCombatPausedByTutorial, IsActorDead(param1), isCombatInTransition, isCombatFinished);
				ofs1 << "PatchedHeatFunc: " << dbg_Counter1++ << dbg_msg << endl;
				if (counter % 2 == 0) {
					ofs2 << "OrigHeatFunc: " << dbg_Counter2++ << dbg_msg << endl;
				}
			}
		}

		if (!IsPlayerInCombat()) {
			counter = 0;
			return; // UpdateHeat() will return immediately in this case, so we just skip calling it
		}
		else if (counter == 1 && (IsCombatInactive() || isCombatPausedByTutorial || IsActorDead(param1) || isCombatInTransition || isCombatFinished)) {
			if (s_Debug) {
				if (ofs2.is_open()) {
					ofs2 << "Fast UpdateHeat: " << dbg_Counter2++ << dbg_msg << endl;
				}
			}
			// UpdateHeat() won't change the Heat value in these cases, but will still execute some code.
			// Since I don't know how important that code is, we'll call UpdateHeat() immediately instead of waiting for the next frame/update.
			origHeatFunc(param1, param2, param3, param4);
			counter++;
		}
		else {
			/*
			* Y3 on PS3 runs at 30 fps during gameplay (60 fps in menus, though that hardly matters here).
			* Y3R always runs its internal logic 60 times per second (even when setting framelimit to 30 fps).
			* While "remastering" Y3, the UpdateHeat function (along with many other functions) wasn't rewritten
			* to take 60 updates per second into account. This leads to at least 2 issues in regards to the Heat bar:
			* - Heat value starts to drop too soon (should start after ~4 seconds but instead starts after ~2 seconds in the "Remaster")
			* - After the Heat value starts dropping, the drain rate is too fast (~2x) when compared to the PS3 version.
			* 
			* Proper way to fix this would be to rewrite the UpdateHeat function (which is responsible for both of these issues)
			* to handle 60 updates per second. But that is definitely more work than I'm willing to put in at the moment.
			* Instead - we'll only run the UpdateHeat function every 2nd time, essentially halving the rate of Heat calculations.
			* This seems to work fine and the resulting drain rate is almost identical to the PS3 original.
			*/
			if (counter % 2 == 0) {
				counter = 0;
				origHeatFunc(param1, param2, param3, param4);
			}
			counter++;
		}
	}
}


void OnInitializeHook()
{
	using namespace std;
	using namespace Memory;
	using namespace hook;
	using namespace config;

	unique_ptr<ScopedUnprotect::Unprotect> Protect = ScopedUnprotect::UnprotectSectionOrFullModule(GetModuleHandle(nullptr), ".text");
	const Config config = loadConfig();
	ofstream ofs = ofstream(format("{:s}{:s}", rsc_Name, ".txt"), ios::binary | ios::trunc | ios::out);

	// Game detection taken from https://github.com/CookiePLMonster/SilentPatchYRC/blob/ae9201926134445f247be42c6f812dc945ad052b/source/SilentPatchYRC.cpp#L396
	enum class Game
	{
		Yakuza3,
		Yakuza4,
		Yakuza5, // Unsupported for now
	} game;
	{
		// "4C 8D 05 ?? ?? ?? ?? 48 8B 15 ?? ?? ?? ?? 33 DB"
		auto gameWindowName = pattern("4C 8D 05 ? ? ? ? 48 8B 15 ? ? ? ? 33 DB").count_hint(1);
		if (gameWindowName.size() == 1)
		{
			// Read the window name from the pointer
			void *match = gameWindowName.get_first(3);

			const char *windowName;
			ReadOffsetValue(match, windowName);
			game = windowName == std::string_view("Yakuza 4") ? Game::Yakuza4 : Game::Yakuza3;
		}
		else
		{
			// Not found? Most likely Yakuza 5
			// Not supported yet
			game = Game::Yakuza5;
		}
	}

	// Check if patch should be disabled
	if ((game != Game::Yakuza3 && !config.Force) || !config.Enable) {
		if (ofs.is_open()) {
			using namespace std::chrono;
			const auto utcNow = system_clock::now();
			const auto str_UtcNow = format("{:%Y/%m/%d %H:%M:%S}", floor<seconds>(utcNow));
			const auto tzNow = HeatFix::tz->to_local(utcNow);
			const auto str_TzNow = format("{:%Y/%m/%d %H:%M:%S}", floor<seconds>(tzNow));
			if (game != Game::Yakuza3) {
				ofs << "Game is NOT Yakuza 3, HeatFix was disabled!" << endl;
			}
			else {
				ofs << "HeatFix was disabled!" << endl;
			}
			if (s_Debug) {
				ofs << endl << format("Config path: \"{:s}\"", config.path) << endl;
			}
			ofs << "Local: " << str_TzNow << endl;
			ofs << "UTC:   " << str_UtcNow << endl;
		}
		return;
	}

	/*
	* Hook/Redirect the game's UpdateHeat function to our own function.
	* As far as I can tell - UpdateHeat is only called while the game is unpaused.
	* Heat gain caused by using an Item (e.g. Staminan) from the menu won't call this function.
	* 
	* SHA-1 checksums for compatible binaries:
	* 20d9614f41dc675848be46920974795481bdbd3b Yakuza3.exe (Steam)
	* 2a55a4b13674d4e62cda2ff86bc365d41b645a92 Yakuza3.exe (Steam without SteamStub)
	* 6c688b51650fa2e9be39e1d934e872602ee54799 Yakuza3.exe (GOG)
	*/
	{
		using namespace HeatFix;
		/*
		* e8 ?? ?? ?? ?? 48 8b 83 d0 13 00 00 f7 80 54 03
		* 
		* Pattern for the player actor's call to UpdateHeat().
		*/
		auto updateHeat = pattern("e8 ? ? ? ? 48 8b 83 d0 13 00 00 f7 80 54 03");
		if (updateHeat.count_hint(1).size() == 1) {
			if (ofs.is_open()) {
				ofs << "Found pattern: UpdateHeat" << endl;
			}
			auto match = updateHeat.get_one();
			Trampoline *trampoline = Trampoline::MakeTrampoline(match.get<void>());
			ReadCall(match.get<void>(), origHeatFunc);
			InjectHook(match.get<void>(), trampoline->Jump(PatchedHeatFunc), PATCH_CALL);
		}

		/*
		* 48 81 ec c0 00 00 00 48 8b d9 e8 ?? ?? ?? ?? 85 c0
		* 
		* UpdateHeat() returns right at the start if this function returns 0.
		* Seems to always return 0, unless the player is in combat.
		*/
		auto playerInCombat = pattern("48 81 ec c0 00 00 00 48 8b d9 e8 ? ? ? ? 85 c0");
		if (playerInCombat.count_hint(1).size() == 1) {
			if (ofs.is_open()) {
				ofs << "Found pattern: IsPlayerInCombat" << endl;
			}
			auto match = playerInCombat.get_one();
			ReadCall(match.get<void>(10), IsPlayerInCombat);
		}

		/*
		* e8 ?? ?? ?? ?? f7 83 d8 13 00 00 00 00 01 00
		* 
		* Most of UpdateHeat()'s code is skipped if this function returns != 0
		* Seems to return 1 if combat is inactive, e.g. during Heat moves/cutscenes
		*/
		auto combatInactive = pattern("e8 ? ? ? ? f7 83 d8 13 00 00 00 00 01 00");
		if (combatInactive.count_hint(1).size() == 1) {
			if (ofs.is_open()) {
				ofs << "Found pattern: IsCombatInactive" << endl;
			}
			auto match = combatInactive.get_one();
			ReadCall(match.get<void>(), IsCombatInactive);
		}

		/*
		* 48 8b 0d ?? ?? ?? ?? 39 41 08 0f 85
		* 
		* Seems to point to a variable that is == 0 while combat is ongoing and == 1
		* when combat is finished (i.e. either the player or all enemies are knocked out)
		*/
		auto globalPattern = pattern("48 8b 0d ? ? ? ? 39 41 08 0f 85");
		if (globalPattern.count_hint(1).size() == 1) {
			if (ofs.is_open()) {
				ofs << "Found pattern: IsCombatFinished" << endl;
			}
			auto match = globalPattern.get_one();
			ReadOffsetValue(match.get<void>(3), globalPointer);
		}
	}

	// log current time to file to get some feedback once hook is done
	if (ofs.is_open()) {
		using namespace std::chrono;
		const auto utcNow = system_clock::now();
		const auto str_UtcNow = format("{:%Y/%m/%d %H:%M:%S}", floor<seconds>(utcNow));
		const auto tzNow = HeatFix::tz->to_local(utcNow);
		const auto str_TzNow = format("{:%Y/%m/%d %H:%M:%S}", floor<seconds>(tzNow));
		ofs << "Hook done!" << endl;
		if (s_Debug) {
			ofs << endl << format("Config path: \"{:s}\"", config.path) << endl;
		}
		ofs << "Local: " << str_TzNow << endl;
		ofs << "UTC:   " << str_UtcNow << endl;
	}
}
