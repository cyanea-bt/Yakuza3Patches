﻿#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#define WINVER 0x0601
#define _WIN32_WINNT 0x0601

#include <format>
#include "ModUtils/MemoryMgr.h"
#include "ModUtils/Trampoline.h"
#include "ModUtils/Patterns.h"
#include "config.h"
#include "utils.h"
#include "Yakuza3.h"

#if _DEBUG
static constexpr bool s_Debug = true;
#else
static constexpr bool s_Debug = false;
#endif // _DEBUG
static config::Config s_Config;


namespace LegacyHeatFix {
	using namespace std;
	static uint64_t dbg_Counter1 = 0, dbg_Counter2 = 0;
	static string dbg_msg;

	/*
	* Parameter types aren't correct but it shouldn't matter for our purposes.
	* param1 is probably a pointer to the player actor object
	* No clue what the other 3 are, but all 4 parameters seem to be 8 bytes wide.
	*/
	typedef void(*UpdateHeatType)(uintptr_t, uintptr_t, uintptr_t, uintptr_t);
	static UpdateHeatType origHeatFunc = nullptr;
	static UpdateHeatType verifyHeatFunc = nullptr;
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
			if (origHeatFunc != verifyHeatFunc || verifyHeatFunc == nullptr) {
				DebugBreak();
			}
			dbg_msg = format("TzNow: {:s} - IsPlayerInCombat: {:d} - IsCombatInactive: {:d} - isCombatPausedByTutorial: {:d} - IsActorDead: {:d} - isCombatInTransition: {:d} - isCombatFinished: {:d}",
				utils::TzString_ms(), IsPlayerInCombat(), IsCombatInactive(), isCombatPausedByTutorial, IsActorDead(param1), isCombatInTransition, isCombatFinished);
			utils::Log(format("PatchedHeatFunc: {:d} - {:s}", dbg_Counter1++, dbg_msg), 1);
			if (counter % 2 == 0) {
				utils::Log(format("OrigHeatFunc: {:d} - {:s}", dbg_Counter2++, dbg_msg), 2);
			}
		}

		if (!IsPlayerInCombat()) {
			counter = 0;
			return; // UpdateHeat() will return immediately in this case, so we just skip calling it
		}
		else if (counter == 1 && (IsCombatInactive() || isCombatPausedByTutorial || IsActorDead(param1) || isCombatInTransition || isCombatFinished)) {
			if (s_Debug) {
				utils::Log(format("Fast UpdateHeat: {:d} - {:s}", dbg_Counter2++, dbg_msg), 2);
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

namespace HeatFix {
	using namespace std;
	static uint64_t dbg_Counter1 = 0, dbg_Counter2 = 0, dbg_Counter3 = 0;

	typedef float(*GetActorFloatType)(void **);
	static GetActorFloatType verifyGetCurHeat = nullptr, verifyGetMaxHeat = nullptr;

	static float GetCurrentHeatValue(void **playerActor) {
		// MOV RAX,qword ptr [param_1]
		// CALL qword ptr [RAX + 0x338]
		uintptr_t *vfTable = (uintptr_t *)*playerActor;
		GetActorFloatType getCurHeat = (GetActorFloatType)vfTable[0x338 / sizeof(uintptr_t)];

		if (s_Debug) {
			if (getCurHeat != verifyGetCurHeat || verifyGetCurHeat == nullptr) {
				DebugBreak();
			}
		}
		float curHeat = getCurHeat(playerActor);

		if (s_Debug) {
			utils::Log(format("{:s} - {:d} - TzNow: {:s} - playerActor: {} - vfTable: {} - getCurHeat: {} - curHeat: {}",
				"GetCurrentHeatValue", dbg_Counter1++, utils::TzString_ms(), (void *)playerActor, (void *)vfTable, (void *)getCurHeat, curHeat), 1);
		}
		return curHeat;
	}

	static float GetMaxHeatValue(void **playerActor) {
		// MOV RAX,qword ptr [param_1]
		// CALL qword ptr [RAX + 0x340]
		uintptr_t *vfTable = (uintptr_t *)*playerActor;
		GetActorFloatType getMaxHeat = (GetActorFloatType)vfTable[0x340 / sizeof(uintptr_t)];

		if (s_Debug) {
			if (getMaxHeat != verifyGetMaxHeat || verifyGetMaxHeat == nullptr) {
				DebugBreak();
			}
		}
		float maxHeat = getMaxHeat(playerActor);

		if (s_Debug) {
			utils::Log(format("{:s} - {:d} - TzNow: {:s} - playerActor: {} - vfTable: {} - getMaxHeat: {} - maxHeat: {}",
				"GetMaxHeatValue", dbg_Counter2++, utils::TzString_ms(), (void *)playerActor, (void *)vfTable, (void *)getMaxHeat, maxHeat), 2);
		}
		return maxHeat;
	}

	static uint16_t GetNewHeatDrainTimer(void **playerActor, uint16_t heatDrainTimer) {
		// MOVZX EDI,word ptr [param_1 + 0x14c8]
		uint16_t oldHeatDrainTimer = *(uint16_t *)((uintptr_t)playerActor + 0x14c8);

		if (s_Debug) {
			utils::Log(format("{:s} - {:d} - TzNow: {:s} - playerActor: {} - oldHeatDrainTimer: {:d} - heatDrainTimer: {:d}", 
				"GetNewHeatDrainTimer", dbg_Counter3++, utils::TzString_ms(), (void *)playerActor, oldHeatDrainTimer, heatDrainTimer), 3);
		}
		//return heatDrainTimer;
		return oldHeatDrainTimer; // for testing purposes - Heat will never drain on its own, since the drain timer won't increase
	}

	static float GetNewHeatValue(void *param1, float heatVal, float unkXMM2, float unkXMM3) {
		(void)param1;
		(void)unkXMM2;
		(void)unkXMM3;
		return heatVal; // not implemented yet
	}
}


void OnInitializeHook()
{
	using namespace std;
	using namespace Memory;
	using namespace hook;

	unique_ptr<ScopedUnprotect::Unprotect> Protect = ScopedUnprotect::UnprotectSectionOrFullModule(GetModuleHandle(nullptr), ".text");
	s_Config = config::GetConfig();

	// Check if patch should be disabled
	if (Yakuza3::ShouldDisablePatch()) {
		return;
	}

	if (s_Config.UseOldPatch) {
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
			using namespace LegacyHeatFix;

			if (s_Debug) {
				// Open debug logfile streams (not necessary but will save some time on the first real log message)
				utils::Log("", 1);
				utils::Log("", 2);

				// UpdateHeat - verify we're calling the correct function
				auto updateHeatCheck = pattern("40 53 48 81 ec c0 00 00 00 48 8b d9 e8");
				if (updateHeatCheck.count_hint(1).size() == 1) {
					auto match = updateHeatCheck.get_one();
					verifyHeatFunc = (UpdateHeatType)match.get<void>();
				}
			}

			/*
			* e8 ?? ?? ?? ?? 48 8b 83 d0 13 00 00 f7 80 54 03
			*
			* Pattern for the player actor's call to UpdateHeat().
			*/
			auto updateHeat = pattern("e8 ? ? ? ? 48 8b 83 d0 13 00 00 f7 80 54 03");
			if (updateHeat.count_hint(1).size() == 1) {
				utils::Log("Found pattern: UpdateHeat");
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
				utils::Log("Found pattern: IsPlayerInCombat");
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
				utils::Log("Found pattern: IsCombatInactive");
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
				utils::Log("Found pattern: IsCombatFinished");
				auto match = globalPattern.get_one();
				ReadOffsetValue(match.get<void>(3), globalPointer);
			}
		}
	}
	else {
		{
			using namespace HeatFix;

			if (s_Debug) {
				// Open debug logfile streams (not necessary but will save some time on the first real log message)
				utils::Log("", 1);
				utils::Log("", 2);
				utils::Log("", 3);

				// GetCurrentHeatValue - verify we're calling the correct function
				auto getCurHeatPattern = pattern("48 8b 81 10 15 00 00 c5 fa 10 40 08 c3");
				if (getCurHeatPattern.count_hint(1).size() == 1) {
					auto match = getCurHeatPattern.get_one();
					verifyGetCurHeat = (GetActorFloatType)match.get<void>();
				}

				// GetMaxHeatValue - verify we're calling the correct function
				auto getMaxHeatPattern = pattern("48 8b 81 10 15 00 00 c5 fa 10 40 0c c3");
				if (getMaxHeatPattern.count_hint(1).size() == 1) {
					auto match = getMaxHeatPattern.get_one();
					verifyGetMaxHeat = (GetActorFloatType)match.get<void>();
				}
			}

			/*
			* c5 e2 5f f2 ff 90 40 03 00 00
			*/
			auto finalHeatCalc = pattern("c5 e2 5f f2 ff 90 40 03 00 00");
			if (finalHeatCalc.count_hint(1).size() == 1) {
				utils::Log("Found pattern: FinalHeatCalc");
				auto match = finalHeatCalc.get_one();
				void *callAddr = match.get<void>(4);
				void *retAddr = match.get<void>(10);
				Trampoline *trampoline = Trampoline::MakeTrampoline(callAddr);
				Nop(callAddr, 6);

				const uint8_t payload[] = {
					0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // movabs rax, GetCurrentHeatValue
					0xff, 0xd0, // call rax
					0x48, 0x89, 0xd9, // mov rcx, rbx (copy address of playerActor to param1)
					0x48, 0x89, 0xfa, // mov rdx, rdi (copy NEW heat drain timer to param2)
					0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // movabs rax, GetNewHeatDrainTimer
					0xff, 0xd0, // call rax
					0x89, 0xc7, // mov edi, eax (overwrite NEW heat drain timer with retVal from GetNewHeatDrainTimer)
					0x48, 0x89, 0xd9, // mov rcx, rbx (copy address of playerActor to param1)
					0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // movabs rax, GetMaxHeatValue
					0xff, 0xd0, // call rax
					0xe9, 0x00, 0x00, 0x00, 0x00 // jmp retAddr
				};
				/*
				* Shortly after we return to the game's own code, the contents of (E)DI will be written to memory
				* as the new value for HeatUpdate()'s drain timer. This is the write instruction:
				* MOV word ptr [RBX + 0x14c8],DI
				*/

				std::byte *space = trampoline->RawSpace(sizeof(payload));
				memcpy(space, payload, sizeof(payload));

				auto pGetCurrentHeatValue = utils::GetFuncAddr(GetCurrentHeatValue);
				memcpy(space + 2, &pGetCurrentHeatValue, sizeof(pGetCurrentHeatValue));

				auto pGetNewHeatDrainTimer = utils::GetFuncAddr(GetNewHeatDrainTimer);
				// 10 + 2 + 3 + 3 + 2 = 20
				memcpy(space + 20, &pGetNewHeatDrainTimer, sizeof(pGetNewHeatDrainTimer));

				auto pGetMaxHeatValue = utils::GetFuncAddr(GetMaxHeatValue);
				// 10 + 2 + 3 + 3 + 10 + 2 + 2 + 3 + 2 = 37
				memcpy(space + 37, &pGetMaxHeatValue, sizeof(pGetMaxHeatValue));

				WriteOffsetValue(space + sizeof(payload) - 4, retAddr);
				InjectHook(callAddr, space, PATCH_JUMP);
			}
		}
	}

	// log current time to file to get some feedback once hook is done
	utils::Log("Hook done!");
	if (s_Debug) {
		utils::Log(format("\nConfig path: \"{:s}\"", s_Config.path));
	}
	utils::Log(format("Local: {:s}", utils::TzString()));
	utils::Log(format("UTC:   {:s}", utils::UTCString()), true);
}
