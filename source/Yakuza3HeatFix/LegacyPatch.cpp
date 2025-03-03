#include "utils.h"
#include "Yakuza3.h"
#include "Yakuza3HeatFix.h"


namespace LegacyHeatFix {
	using namespace std;
	/*
	* Parameter types aren't correct but it shouldn't matter for our purposes.
	* param1 is probably a pointer to the player actor object
	* No clue what the other 3 are, but all 4 parameters seem to be 8 bytes wide.
	*/
	typedef void (*UpdateHeatType)(uintptr_t, uintptr_t, uintptr_t, uintptr_t);
	static UpdateHeatType origHeatFunc = nullptr;
	static UpdateHeatType verifyHeatFunc = nullptr;
	static int counter = 0;

	static uint32_t(*IsPlayerInCombat)();
	static uintptr_t(*IsCombatInactive)();

	typedef uint32_t (*IsActorDeadType)(uintptr_t);
	static IsActorDeadType IsActorDead = nullptr;

	static uintptr_t globalPointer = 0;
	static uintptr_t pIsCombatFinished = 0;
	static uintptr_t isCombatFinished = 0;

	static uint32_t isCombatPausedByTutorial = 0;
	static uint32_t isCombatInTransition = 0;

	static bool didNotProcessGrabHit = false;
	static bool didNotProcessRegHit = false;
	static bool didNotProcessEnemyHit = false;
	static bool didNotProcessBlock = false;

	static void PatchedHeatFunc(uintptr_t param1, uintptr_t param2, uintptr_t param3, uintptr_t param4) {
		string dbg_msg;
		HeatFix::PlayerActor = (void **)param1; // save address of player actor for use in PatchedGetDisplayString()

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

		// VMOVSS  XMM0,dword ptr [param_1 + 0x1cc4]
		// VCOMISS XMM0,0.0f
		// JBE     +0x36
		const float grabDamage = *(float *)(param1 + 0x1cc4); // I assume that is what this float is
		const bool successfulGrabHit = grabDamage > 0.0f;

		// CMP  word ptr [param_1 + 0x1abe],0x0
		// JBE  +0x63
		const uint16_t unkUint1 = *(uint16_t *)(param1 + 0x1abe);
		// TEST byte ptr [param_1 + 0x1a9c],0x10
		// JNZ  +0x5a
		const uint8_t unkUint2 = *(uint8_t *)(param1 + 0x1a9c);
		const bool successfulRegularHit = !successfulGrabHit && (unkUint1 > 0 && (unkUint2 & 0x10) == 0);

		// MOVZX EDI,word ptr [param_1 + 0x14c8]
		const uint16_t heatDrainTimer = *(uint16_t *)(param1 + 0x14c8);

		// movzx eax,word ptr [param_1 + 0x1abc]
		const uint16_t incomingDamage = *(uint16_t *)(param1 + 0x1abc);

		// CMP word ptr [RBX + 0x1ac0],SI
		const uint16_t blockedDamage = *(uint16_t *)(param1 + 0x1ac0);

		if (isDEBUG) {
			if (origHeatFunc != verifyHeatFunc || verifyHeatFunc == nullptr) {
				DebugBreak();
				utils::Log("");
			}

			if (didNotProcessGrabHit && (!successfulGrabHit || heatDrainTimer > 0x1)) {
				// Does indeed happen
				utils::Log(""); // Should have reset drain timer on last frame but missed it due to 30fps cap on UpdateHeat
			}
			else if (didNotProcessRegHit && (!successfulRegularHit || heatDrainTimer > 0x1)) {
				// Does indeed happen
				utils::Log(""); // Should have reset drain timer on last frame but missed it due to 30fps cap on UpdateHeat
			}

			// Important when blocking hits with unlocked "Tortoise Spirit" upgrade
			if (didNotProcessBlock && blockedDamage == 0) {
				// Does indeed happen
				utils::Log(""); // Should have reset drain timer on last frame but missed it due to 30fps cap on UpdateHeat
			}

			// Wasn't able to trigger this one but I am certain it does happen at least occasionally.
			if (didNotProcessEnemyHit && incomingDamage == 0) {
				utils::Log(""); // Should have subtracted Heat on last frame but missed it due to 30fps cap on UpdateHeat
			}

			dbg_msg = fmt::format("IsPlayerInCombat: {:d} - IsCombatInactive: {:d} - isCombatPausedByTutorial: {:d} - IsActorDead: {:d} - isCombatInTransition: {:d} - isCombatFinished: {:d}",
				IsPlayerInCombat(), IsCombatInactive(), isCombatPausedByTutorial, IsActorDead(param1), isCombatInTransition, isCombatFinished);
			utils::Log(dbg_msg, 1);
			if (counter % 2 == 0) {
				utils::Log(dbg_msg, 2);
			}
		}

		if (!IsPlayerInCombat()) {
			counter = 0;
			return; // UpdateHeat() will return immediately in this case, so we just skip calling it
		}
		else if (counter == 1 && (IsCombatInactive() || isCombatPausedByTutorial || IsActorDead(param1) || isCombatInTransition || isCombatFinished)) {
			if (isDEBUG) {
				utils::Log(fmt::format("Fast UpdateHeat >>>>>> {:s}", dbg_msg), 2);
			}
			// UpdateHeat() won't change the Heat value in these cases, but will still execute some code.
			// Since I don't know how important that code is, we'll call UpdateHeat() immediately instead of waiting for the next frame/update.
			origHeatFunc(param1, param2, param3, param4);
			counter++;
			didNotProcessGrabHit = false;
			didNotProcessRegHit = false;
			didNotProcessEnemyHit = false;
			didNotProcessBlock = false;
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
				origHeatFunc(param1, param2, param3, param4);
				counter = 0;
				didNotProcessGrabHit = false;
				didNotProcessRegHit = false;
				didNotProcessEnemyHit = false;
				didNotProcessBlock = false;
			}
			else {
				if (successfulGrabHit) {
					didNotProcessGrabHit = true;
				}
				else if (successfulRegularHit) {
					didNotProcessRegHit = true;
				}
				if (blockedDamage > 0) {
					didNotProcessBlock = true;
				}
				if (incomingDamage > 0) {
					didNotProcessEnemyHit = true;
				}
			}
			counter++;
		}
	}


	void InitPatch() {
		using namespace std;
		using namespace Memory;
		using namespace hook;

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
		if (isDEBUG) {
			// Open debug logfile streams (not necessary but will save some time on the first real log message)
			utils::Log("", 1, "PatchedHeatFunc");
			utils::Log("", 2, "OrigHeatFunc");

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
