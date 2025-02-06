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
	static IsActorDeadType IsActorDead_byRef = nullptr;
	static IsActorDeadType IsActorDead_byPattern;

	static uintptr_t globalPointer = 0;
	static uintptr_t globalAddress = 0;
	static uintptr_t isCombatFinished = 0;

	static uint32_t unknownCondition = 0; // No clue when this one is != 0
	static uint32_t IsCombatInTransition = 0; // Seems to be != 0 during combat intro or when fading to black after the player dies

	static void PatchedHeatFunc(uintptr_t param1, uintptr_t param2, uintptr_t param3, uintptr_t param4) {
		if (IsActorDead_byRef == nullptr) {
			// MOV  RDX,qword ptr [param_1]
			// CALL qword ptr [RDX + 0x268]
			uintptr_t pIsActorDead = *(uintptr_t *)(param1);
			pIsActorDead = *(uintptr_t *)(pIsActorDead + 0x268);
			IsActorDead_byRef = (IsActorDeadType)pIsActorDead;

			if (IsActorDead_byRef != IsActorDead_byPattern) {
				logFailed = !logFailed;
				logFailed = !logFailed; // just need something to put a breakpoint on
			}
		}

		if (globalAddress == 0) {
			globalAddress = *(uintptr_t *)globalPointer;
		}
		isCombatFinished = *(uintptr_t *)(globalAddress + 8);

		// MOV  RBX,param_1
		// TEST dword ptr [RBX + 0x13d8],0x10000
		unknownCondition = *(uint32_t *)(param1 + 0x13d8);
		unknownCondition &= 0x10000;

		// MOV  RBX,param_1
		// MOV  RCX,qword ptr [RBX + 0x13d0]
		// TEST dword ptr [RCX + 0x354],0x4000000
		uintptr_t pIsCombatInTransition = *(uintptr_t *)(param1 + 0x13d0);
		IsCombatInTransition = *(uint32_t *)(pIsCombatInTransition + 0x354);
		IsCombatInTransition &= 0x4000000;

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
				ofs1 << "PatchedHeatFunc: " << str_TzNow << " - " << dbg_Counter1++ << format(" - IsPlayerInCombat: {:d} - IsCombatInactive: {:d} - IsActorDead: {:d}", IsPlayerInCombat(), IsCombatInactive(), IsActorDead_byRef(param1)) << endl;
				if (counter % 2 == 0) {
					ofs2 << "OrigHeatFunc: " << str_TzNow << " - " << dbg_Counter2++ << format(" - IsPlayerInCombat: {:d} - IsCombatInactive: {:d} - IsActorDead: {:d}", IsPlayerInCombat(), IsCombatInactive(), IsActorDead_byRef(param1)) << endl;
				}
				if (IsCombatInactive() != 0) {
					logFailed = !logFailed;
					logFailed = !logFailed; // just need something to put a breakpoint on
				}
				if (IsActorDead_byRef(param1) != 0) {
					logFailed = !logFailed;
					logFailed = !logFailed; // just need something to put a breakpoint on
				}
				if (isCombatFinished != 0) {
					logFailed = !logFailed;
					logFailed = !logFailed; // just need something to put a breakpoint on
				}
				if (IsPlayerInCombat() && unknownCondition != 0) {
					logFailed = !logFailed;
					logFailed = !logFailed; // just need something to put a breakpoint on
				}
				if (IsPlayerInCombat() && IsCombatInTransition != 0) {
					logFailed = !logFailed;
					logFailed = !logFailed; // just need something to put a breakpoint on
				}
			}
		}
		/*
		* Y3 on PS3 runs at 30 fps during gameplay (60 fps in menus, though that hardly matters here).
		* Y3R always runs its internal logic 60 times per second (even when setting framelimit to 30 fps).
		* While "remastering" Y3, the HeatUpdate function (along with many other functions) wasn't rewritten
		* to take 60 updates per second into account. This leads to at least 2 issues in regards to the Heat bar:
		* - Heat value starts to drop too soon (should start after ~4 seconds but instead starts after ~2 seconds in the "Remaster")
		* - After the Heat value starts dropping, the drain rate is too fast (~2x) when compared to the PS3 version.
		* 
		* Proper way to fix this would be to rewrite the HeatUpdate function (which is responsible for both of these issues)
		* to handle 60 updates per second. But that is definitely more work than I'm willing to put in at the moment.
		* Instead - we'll only run the HeatUpdate function every 2nd time, essentially halving the rate of Heat calculations.
		* This seems to work fine and the resulting drain rate is almost identical to the PS3 original.
		*/
		if (counter % 2 == 0) {
			counter = 0;
			origHeatFunc(param1, param2, param3, param4);
		}
		counter++;
	}
}


void OnInitializeHook()
{
	using namespace std;
	using namespace Memory;
	using namespace hook;

	unique_ptr<ScopedUnprotect::Unprotect> Protect = ScopedUnprotect::UnprotectSectionOrFullModule(GetModuleHandle(nullptr), ".text");
	ofstream ofs = ofstream("Yakuza3HeatFix.txt", ios::binary | ios::trunc | ios::out);
	// Check if patch should be disabled
	{
		namespace fs = std::filesystem;
		fs::path manualDisable1("Yakuza3HeatFix.Disable");
		fs::path manualDisable2("Yakuza3HeatFix.disable");
		fs::path manualDisable3("Yakuza3HeatFix.Disable.txt");
		fs::path manualDisable4("Yakuza3HeatFix.disable.txt");
		if (fs::exists(manualDisable1) || fs::exists(manualDisable2) || fs::exists(manualDisable3) || fs::exists(manualDisable4)) {
			using namespace std::chrono;

			if (ofs.is_open()) {
				const auto utcNow = system_clock::now();
				const auto str_UtcNow = format("{:%Y/%m/%d %H:%M:%S}", floor<seconds>(utcNow));
				const auto tzNow = HeatFix::tz->to_local(utcNow);
				const auto str_TzNow = format("{:%Y/%m/%d %H:%M:%S}", floor<seconds>(tzNow));
				ofs << "HeatFix was disabled!" << endl;
				ofs << "Local: " << str_TzNow << endl;
				ofs << "UTC:   " << str_UtcNow << endl;
			}
			return;
		}
	}

	// Hook/Redirect the game's HeatUpdate function to our own function.
	// As far as I can tell - HeatUpdate is only called while the game is unpaused.
	// So Heat gain caused by using an Item (e.g. Staminan) from the menu won't call this function.
	{
		using namespace HeatFix;
		/*
		* e8 ?? ?? ?? ?? 48 8b 83 d0 13 00 00 f7 80 54 03
		* Pattern works for Steam and GOG versions.
		* 
		* Sha1 checksums for binaries:
		* 20d9614f41dc675848be46920974795481bdbd3b Yakuza3.exe (Steam)
		* 2a55a4b13674d4e62cda2ff86bc365d41b645a92 Yakuza3.exe (Steam without SteamStub)
		* 6c688b51650fa2e9be39e1d934e872602ee54799 Yakuza3.exe (GOG)
		*/
		auto updateHeat = pattern("e8 ? ? ? ? 48 8b 83 d0 13 00 00 f7 80 54 03");
		if (updateHeat.count_hint(1).size() == 1) {
			if (ofs.is_open()) {
				ofs << "Found pattern: HeatUpdate" << endl;
			}
			auto match = updateHeat.get_one();
			Trampoline *trampoline = Trampoline::MakeTrampoline(match.get<void>());
			ReadCall(match.get<void>(), origHeatFunc);
			InjectHook(match.get<void>(), trampoline->Jump(PatchedHeatFunc), PATCH_CALL);
		}

		/*
		* 48 81 ec c0 00 00 00 48 8b d9 e8 ?? ?? ?? ?? 85 c0
		* Pattern works for Steam and GOG versions.
		* 
		* HeatUpdate() returns right at the start if this function returns 0.
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
		* Pattern works for Steam and GOG versions.
		* 
		* Most of HeatUpdate()'s code is skipped if this function returns != 0
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
		* 8b 81 d8 13 00 00 c1 e8 1e 83 e0 01
		* Pattern works for Steam and GOG versions.
		* 
		* Almost all of HeatUpdate()'s code is skipped if this function returns != 0
		* Seems to return 1 if the player is dead.
		* Don't necessarily have to get this via pattern, since we can get the address from the player object during HeatUpdate()
		*/
		auto actorDead = pattern("8b 81 d8 13 00 00 c1 e8 1e 83 e0 01");
		if (actorDead.count_hint(1).size() == 1) {
			if (ofs.is_open()) {
				ofs << "Found pattern: IsActorDead" << endl;
			}
			auto match = actorDead.get_one();
			IsActorDead_byPattern = (IsActorDeadType)match.get<void>();
		}

		/*
		* 48 8b 0d ?? ?? ?? ?? 39 41 08 0f 85
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
	{
		using namespace std::chrono;

		if (ofs.is_open()) {
			const auto utcNow = system_clock::now();
			const auto str_UtcNow = format("{:%Y/%m/%d %H:%M:%S}", floor<seconds>(utcNow));
			const auto tzNow = HeatFix::tz->to_local(utcNow);
			const auto str_TzNow = format("{:%Y/%m/%d %H:%M:%S}", floor<seconds>(tzNow));
			ofs << "Hook done!" << endl;
			ofs << "Local: " << str_TzNow << endl;
			ofs << "UTC:   " << str_UtcNow << endl;
		}
	}
}
