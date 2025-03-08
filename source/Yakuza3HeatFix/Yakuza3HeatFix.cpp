#include "utils.h"
#include "win_utils.h"
#include "Yakuza3.h"
#include "Yakuza3HeatFix.h"


namespace HeatFix {
	using namespace std;
	static uint8_t drainTimeLimiter = 1;
	static uint16_t lastDrainTimer = 0, substituteTimer = 0;
	static constexpr uint16_t MAX_DrainTimer = 0x73;
	static constexpr float floatMulti = 1.0f / 2.0f; // for float values that have to be halved when running at 60 fps
	static constexpr uint8_t integerDiv = 2; // for integer values that have to be halved when running at 60 fps

	typedef bool (*GetActorBoolType)(void **);
	typedef float (*GetActorFloatType)(void **);
	typedef uint8_t(*AddSubtractHeatType)(void **, int32_t);
	static GetActorBoolType verifyIsPlayerDrunk = nullptr;
	static GetActorFloatType verifyGetCurHeat = nullptr, verifyGetMaxHeat = nullptr;
	static AddSubtractHeatType verifyAddHeat = nullptr;

	// for debugging purposes
	static uint8_t AddSubtractHeatWrapper(void **playerActor, int32_t amount) {
		uintptr_t *vfTable = (uintptr_t *)*playerActor;
		const AddSubtractHeatType AddHeat = (AddSubtractHeatType)vfTable[0x318 / sizeof(uintptr_t)];
		const GetActorFloatType GetCurHeat = (GetActorFloatType)vfTable[0x338 / sizeof(uintptr_t)];
		const GetActorFloatType GetMaxHeat = (GetActorFloatType)vfTable[0x340 / sizeof(uintptr_t)];
		if (isDEBUG) {
			if (AddHeat != verifyAddHeat) {
				DebugBreak(); // should never happen
				utils::Log("");
			}
		}

		const float heatBefore = GetCurHeat(playerActor);
		const float maxHeat = GetMaxHeat(playerActor);
		const uint8_t result = AddHeat(playerActor, amount);
		const float heatAfter = GetCurHeat(playerActor);

		if (isDEBUG) {
			const float heatDiff = heatAfter - heatBefore;
			utils::Log(9,
				"amount: {:d} - result: {:d} - maxHeat: {:.3f} - heatBefore: {:.3f} - heatAfter: {:.3f} - heatDiff: {:.3f}",
				amount, result, maxHeat, heatBefore, heatAfter, heatDiff
			);

			DebugBreak();
			utils::Log("");
			winutils::showErrorMessage("AddSubtractHeatWrapper"); // In case I don't have a debugger attached
		}

		return result;
	}

	static uint8_t AddHeatHoldFinisher(void **playerActor, int32_t amount) {
		uintptr_t *vfTable = (uintptr_t *)*playerActor;
		const AddSubtractHeatType AddHeat = (AddSubtractHeatType)vfTable[0x318 / sizeof(uintptr_t)];
		const GetActorFloatType GetCurHeat = (GetActorFloatType)vfTable[0x338 / sizeof(uintptr_t)];
		const GetActorFloatType GetMaxHeat = (GetActorFloatType)vfTable[0x340 / sizeof(uintptr_t)];
		if (isDEBUG) {
			if (AddHeat != verifyAddHeat) {
				DebugBreak(); // should never happen
				utils::Log("");
			}
		}

		/*
		* VMOVSS XMM1,dword ptr [playerActor + 0x1ab0]
		* (will be converted to int right before the call to this function)
		* 
		* floatAmount seems to always be 8.3333333f during hold combo finishers.
		* Instructions that set this value start at Yakuza3.exe+0x4214aa (Steam version)
		* Pattern to find it in other versions: c5 fa 11 8b b0 1a 00 00
		* 
		* Interestingly that function is used by the player and enemies and is called for pretty
		* much every attack? In the player's case it seems to set [playerActor + 0x1ab0] to 4.1777f
		* for light attacks and 8.3333f for heavy attacks.
		*/
		const float floatAmount = *(float *)((uintptr_t)playerActor + 0x1ab0);

		const float heatBefore = GetCurHeat(playerActor);
		const float maxHeat = GetMaxHeat(playerActor);
		const uint8_t result = AddHeat(playerActor, amount);
		// Should be safer than trying to check for 8.3333 repeating
		if (floatAmount > 8.0f && floatAmount < 8.5f) {
			// result will only be 0 IF Heat is already full before calling AddSubtractHeat()
			if (result != 0) {
				// MOV    RAX,qword ptr [playerActor + 0x1510]
				// VMOVSS dword ptr [RAX + 0x8],XMM3
				void *pHeatValues = *(void **)((uintptr_t)playerActor + 0x1510);
				float *pCurHeat = (float *)pHeatValues + (0x8 / sizeof(float));
				if (isDEBUG) {
					if (*pCurHeat != GetCurHeat(playerActor)) {
						DebugBreak(); // should never happen
						utils::Log("");
					}
				}

				/*
				* Try to add the remaining 0.5f (that got truncated by converting floatAmount to int)
				* 8.5f (and NOT 8.33f) is the correct value to add here, since the PS3 version adds 17.0f
				* per frame while running at half of Y3R's framerate. -> 17.0f / 2 = 8.5f
				* Read further down below (where the call to this function is injected) for more info.
				*/
				*pCurHeat = (*pCurHeat + 0.5f) > maxHeat ? maxHeat : (*pCurHeat + 0.5f);
			}
		}
		else if (floatAmount != 10.0f || amount != 10) {
			// amount can be 10 if using the "Yakuza 3 Rebalanced" mod
			// amounts other than 8/10 would be news to me, so break here
			if (isDEBUG) {
				DebugBreak(); // should never happen
				utils::Log("");
			}
		}
		const float heatAfter = GetCurHeat(playerActor);

		if (isDEBUG) {
			const float heatDiff = heatAfter - heatBefore;
			if (heatAfter > maxHeat) {
				DebugBreak(); // should never happen
				utils::Log("");
			}

			utils::Log(8,
				"floatAmount: {:.3f} - amount: {:d} - result: {:d} - maxHeat: {:.3f} - heatBefore: {:.3f} - heatAfter: {:.3f} - heatDiff: {:.3f}",
				floatAmount, amount, result, maxHeat, heatBefore, heatAfter, heatDiff
			);
		}

		return result;
	}

	static float GetCurrentHeatValue(void **playerActor) {
		// MOV RAX,qword ptr [param_1]
		// CALL qword ptr [RAX + 0x338]
		uintptr_t *vfTable = (uintptr_t *)*playerActor;
		GetActorFloatType GetCurHeat = (GetActorFloatType)vfTable[0x338 / sizeof(uintptr_t)];

		if (isDEBUG) {
			if (GetCurHeat != verifyGetCurHeat || verifyGetCurHeat == nullptr) {
				DebugBreak(); // should never happen
				utils::Log("");
			}
		}
		const float curHeat = GetCurHeat(playerActor);

		if (isDEBUG) {
			utils::Log(1,
				"playerActor: {:p} - vfTable: {:p} - GetCurHeat: {:p} - curHeat: {:.3f}",
				(void *)playerActor, (void *)vfTable, (void *)GetCurHeat, curHeat
			);
		}
		return curHeat;
	}

	static float GetMaxHeatValue(void **playerActor) {
		// MOV RAX,qword ptr [param_1]
		// CALL qword ptr [RAX + 0x340]
		uintptr_t *vfTable = (uintptr_t *)*playerActor;
		GetActorFloatType GetMaxHeat = (GetActorFloatType)vfTable[0x340 / sizeof(uintptr_t)];

		if (isDEBUG) {
			if (GetMaxHeat != verifyGetMaxHeat || verifyGetMaxHeat == nullptr) {
				DebugBreak(); // should never happen
				utils::Log("");
			}
		}
		const float maxHeat = GetMaxHeat(playerActor);

		if (isDEBUG) {
			utils::Log(2,
				"playerActor: {:p} - vfTable: {:p} - GetMaxHeat: {:p} - maxHeat: {:.3f}",
				(void *)playerActor, (void *)vfTable, (void *)GetMaxHeat, maxHeat
			);
		}
		return maxHeat;
	}

	static bool IsPlayerDrunk(void **playerActor) {
		// MOV RAX,qword ptr [param_1]
		// CALL qword ptr [RAX + 0x290]
		uintptr_t *vfTable = (uintptr_t *)*playerActor;
		GetActorBoolType IsDrunk = (GetActorBoolType)vfTable[0x290 / sizeof(uintptr_t)];

		if (isDEBUG) {
			if (IsDrunk != verifyIsPlayerDrunk || verifyIsPlayerDrunk == nullptr) {
				DebugBreak(); // should never happen
				utils::Log("");
			}
		}
		return IsDrunk(playerActor);
	}

	static bool PatchedIsPlayerDrunk(void **playerActor, const float newHeatVal, const uint16_t newDrainTimer) {
		if (isDEBUG) {
			// movzx eax,word ptr [param_1 + 0x1abc]
			const uint16_t incomingDamage = *(uint16_t *)((uintptr_t)playerActor + 0x1abc);

			const float oldHeatVal = GetCurrentHeatValue(playerActor);
			if (newHeatVal != oldHeatVal && newDrainTimer != 0) {
				// AFAIK - Heat value should be unchanged at this point, unless player and enemy hit each other on the same frame
				// In that rare case (newDrainTimer) should be == 0, since the player hitting the enemy resets it to 0.
				DbgBreak();
				spdlog::error(
					"{:s} - oldHeatVal: {:.3f} - newHeatVal: {:.3f} - newDrainTimer: {:d}",
					"PatchedIsPlayerDrunk", oldHeatVal, newHeatVal, newDrainTimer
				);
			}

			utils::Log(6,
				"playerActor: {:p} - oldHeatVal: {:.3f} - newHeatVal: {:.3f} - newDrainTimer: {:d} - incomingDamage: {:d}",
				(void *)playerActor, oldHeatVal, newHeatVal, newDrainTimer, incomingDamage
			);
		}

		return IsPlayerDrunk(playerActor);
	}


	typedef char* (*GetDisplayStringType)(void *, uint32_t, uint32_t);
	static GetDisplayStringType origGetDisplayString = nullptr;
	static string replaceStr("Cool replacement string");

	static char* PatchedGetDisplayString(void *param1, uint32_t param2, uint32_t param3) {
		char *retVal = origGetDisplayString(param1, param2, param3);
		const string str(retVal);
		if (isDEBUG) {
			utils::Log(7,
				"param1: {:p} - param2: {:d} - param3: {:d} - pStr: {:p} - str: {:s}",
				param1, param2, param3, (void *)retVal, str
			);
		}

		// param3 is always 1?
		if (PlayerActor != nullptr && !str.empty()) {
			// MOVZX EDI,word ptr [PlayerActor + 0x14c8]
			const uint16_t heatDrainTimer = *(uint16_t *)((uintptr_t)PlayerActor + 0x14c8);

			// CALL qword ptr [PlayerActor + 0x338]
			uintptr_t *vfTable = (uintptr_t *)*PlayerActor;
			GetActorFloatType GetCurHeat = (GetActorFloatType)vfTable[0x338 / sizeof(uintptr_t)];
			const float heatValue = GetCurHeat(PlayerActor);

			replaceStr = fmt::cformat("Drain Timer: {:03d}/{:d} ; Current Heat: {:08.2f}", heatDrainTimer, MAX_DrainTimer, heatValue);
			retVal = replaceStr.data();
		}
		return retVal;
	}


	static uint16_t GetNewHeatDrainTimer(void **playerActor, const uint16_t curDrainTimer) {
		uint16_t newDrainTimer;
		const uint16_t MAX_SubstituteTimer = (CONFIG.DrainTimeMulti * 2 - 2); // -2 because timer starts at 0
		uint8_t oldDrainTimeLimiter; // for logging purposes
		uint16_t oldSubstituteTimer; // for logging purposes
		if (isDEBUG) {
			oldDrainTimeLimiter = drainTimeLimiter;
			oldSubstituteTimer = substituteTimer;

			// movzx eax,word ptr [param_1 + 0x1abc]
			const uint16_t incomingDamage = *(uint16_t *)((uintptr_t)playerActor + 0x1abc);
			if (incomingDamage > 0) {
				DebugBreak(); // Drain timer shouldn't change on a frame with incoming damage.
				utils::Log("");
			}
		}

		if (CONFIG.DisableHeatDrain) {
			newDrainTimer = 0; // Heat will never drain on its own
		}
		else if (curDrainTimer != lastDrainTimer || curDrainTimer == 0) {
			// Drain timer got reset somewhere else
			drainTimeLimiter = 1;
			substituteTimer = 0;
			// Immediately increase timer to 1, so we'll be able to notice if it gets reset.
			// Otherwise we wouldn't be able to tell whether we kept the timer at 0 intentionally
			// or if it was reset to 0 externally. (Which could lead to starting Heat drain too early)
			newDrainTimer = 1;

			if (isDEBUG) {
				if (curDrainTimer != 0) {
					DebugBreak(); // should never happen
					utils::Log("");
				}
				oldDrainTimeLimiter = drainTimeLimiter;
				oldSubstituteTimer = substituteTimer;
			}			
		}
		else if (curDrainTimer == 1) {
			// Keep drain timer at 1 for (drainTimeMulti * 2 - 1) frames,
			// to make up for immediately increasing it from 0 to 1 earlier.
			newDrainTimer = 1;
			if (substituteTimer == MAX_SubstituteTimer) { 
				newDrainTimer++;
			}
			else {
				substituteTimer++;
			}

			if (isDEBUG) {
				if (substituteTimer > MAX_SubstituteTimer || drainTimeLimiter != 1) {
					DebugBreak(); // should never happen
					utils::Log("");
				}
			}
		}
		else {
			if (isDEBUG) {
				if (substituteTimer != MAX_SubstituteTimer || substituteTimer == 0) {
					DebugBreak(); // should never happen
					utils::Log("");
				}
			}

			// Increase drain timer every (drainTimeMulti) frames
			newDrainTimer = curDrainTimer;
			if (drainTimeLimiter == CONFIG.DrainTimeMulti) {
				drainTimeLimiter = 1;
				if (newDrainTimer < MAX_DrainTimer) {
					newDrainTimer++;
				}
			}
			else {
				drainTimeLimiter++;
			}

			if (isDEBUG) {
				if (drainTimeLimiter > CONFIG.DrainTimeMulti || newDrainTimer > MAX_DrainTimer) {
					DebugBreak(); // should never happen
					utils::Log("");
				}
			}
		}

		if (isDEBUG) {
			utils::Log(3,
				"playerActor: {:p} - drainTimeMulti: {:d} - drainTimeLimiter: {:d} - curDrainTimer: {:d} - newDrainTimer: {:d} - substituteTimer: {:d}", 
				(void *)playerActor, CONFIG.DrainTimeMulti, oldDrainTimeLimiter, curDrainTimer, newDrainTimer, oldSubstituteTimer
			);
		}
		lastDrainTimer = newDrainTimer;
		return newDrainTimer;
	}

	static float GetNewHeatValue(void **playerActor, const float newHeatVal, const float baseDrainRate, const uint16_t newDrainTimer) {
		const float oldHeatVal = GetCurrentHeatValue(playerActor);
		const float heatDiff = std::fabs(oldHeatVal - newHeatVal);

		// movzx eax,word ptr [param_1 + 0x1abc]
		const uint16_t incomingDamage = *(uint16_t *)((uintptr_t)playerActor + 0x1abc);

		/*
		* There are only 2 ways in which Heat can be lowered by UpdateHeat():
		* 1. Subtracting a fixed amount when hit/damaged by the enemy
		* 2. Continuous drain after the drain timer hits its maximum value
		* 
		* The first one works as intended in Y3R. Each hit is registered for a single
		* frame and only subtracts a fixed amount from the Heat value on the same frame.
		* The second one drains Heat at twice the intended/PS3 rate.
		* 
		* Conveniently - only one of these can happen on any particular frame and Heat
		* loss from taking damage always takes priority over "regular" Heat drain.
		* So by verifying that we didn't get damaged on this frame, we can halve the
		* continuous drain rate without impacting anything else.
		* 
		* The rules for gaining Heat are a bit more involved. Which is why I've chosen to
		* patch sources of Heat gain right where they happen. So by the time we get to this
		* function here, Heat gain will already have been handled/corrected.
		*/
		float retHeatVal = newHeatVal;
		if (newHeatVal < oldHeatVal) {
			if (incomingDamage == 0) {
				if (isDEBUG) {
					if (newDrainTimer != MAX_DrainTimer || baseDrainRate == 0.0f) {
						DbgBreak(); // should never happen
						spdlog::error("{:s} - newDrainTimer: {:d} - baseDrainRate: {:.3f}", "GetNewHeatValue_1", newDrainTimer, baseDrainRate);
					}
				}

				// halve rate of Heat drain
				retHeatVal = oldHeatVal - (heatDiff * floatMulti);
			}
			if (retHeatVal < 0.0f) {
				// newHeatVal parameter can be negative if the current frame's Heat loss exceeds
				// the remaining amount of Heat. The game expects us to check for negative values
				// and return 0.0f in that case.
				retHeatVal = 0.0f;
			}
		}

		if (CONFIG.InfiniteHeat) {
			retHeatVal = GetMaxHeatValue(playerActor);
		}
		else if (CONFIG.ZeroHeat) {
			retHeatVal = 0.0f;
		}

		if (isDEBUG) {
			const bool isDrunk = IsPlayerDrunk(playerActor);
			// MOV RBX,dword ptr [param_1 + 0x1a3c]
			const uint32_t playerStatus = *(uint32_t *)((uintptr_t)playerActor + 0x1a3c);
			switch (playerStatus) {
			case 0:
				utils::Log(""); break; // default value?
			case 1:
				utils::Log(""); break; // player locks on to the enemy?
			case 2:
				utils::Log(""); break; // player uses LH, HH, Grab (but misses the grab?)
			case 3:
				utils::Log(""); break; // player gets hit by the enemy
			case 4:
				utils::Log(""); break; // player gets knocked down by the enemy?
			case 5:
				utils::Log(""); break; // player uses dodge
			case 6:
				utils::Log(""); break; // player is lying on the ground?
			case 7:
				utils::Log(""); break; // player uses block
			case 8:
				utils::Log(""); break; // player picks up an object/weapon?
			case 9:
				utils::Log(""); break; // player grabs the enemy
			case 10:
				utils::Log(""); break; // enemy grabs the player
			case 11:
				utils::Log(""); break; // player uses HH on grabbed enemy?
			case 12:
				utils::Log(""); break; // enemy escapes the player's grab?
			case 15:
				utils::Log(""); break; // player starts Heat move?
			case 16:
				utils::Log(""); break; // enemy throws the player
			case 17:
				utils::Log(""); break; // player tries to throw the enemy, player uses LH on grabbed enemy
			case 19:
				utils::Log(""); break; // player uses taunt
			default:
				DbgBreak();
				spdlog::warn("{:s} - isDrunk: {} - playerStatus: {:d}", "GetNewHeatValue_2", isDrunk, playerStatus);
				break;
			}

			// CMP  byte ptr [RBX + 0x1a49],0x3
			// JNZ +0x0e
			// CMP  byte ptr [RBX + 0x1a48],0x3c
			// JA  +0x05
			const uint8_t unkUInt1 = *(uint8_t *)((uintptr_t)playerActor + 0x1a49);
			const uint8_t unkUInt2 = *(uint8_t *)((uintptr_t)playerActor + 0x1a48);
			if (unkUInt1 == 0) { // default value?
				utils::Log("");
			}
			else if (unkUInt1 == 1) { // while player is standing up?
				utils::Log("");
			}
			else if (unkUInt1 == 2) { // while player is getting knocked down/falling to the ground
				utils::Log("");
			}
			else if (unkUInt1 == 3) { // while player is lying on the ground?
				utils::Log("");
			}
			else if (unkUInt1 == 4) { // while player is locked on to the enemy?
				utils::Log("");
			}
			else if (unkUInt1 == 8) { // "Komaki Cat-Like Reflexes" (i.e. doing a backflip instead of falling down)
				utils::Log("");
			}
			else if (unkUInt1 > 4 && unkUInt1 != 8) {
				DbgBreak();
				spdlog::warn("{:s} - unkUInt1: {:d} - unkUInt2: {:d}", "GetNewHeatValue_3", unkUInt1, unkUInt2);
			}
			if (unkUInt2 > 0x3c) {
				// Happens after some enemy throws/hits? Pretty rare though (only seen a few times so far)
				// Seems to be the time (in frames) until the player stands up from the ground?
				DbgBreak();
				spdlog::warn("{:s} - unkUInt1: {:d} - unkUInt2: {:d}", "GetNewHeatValue_4", unkUInt1, unkUInt2);
			}
			const bool playerGotKnockedDown = (playerStatus == 4) && (unkUInt1 == 0x3) && (unkUInt2 <= 0x3c);
			if (playerStatus == 4) {
				utils::Log(4,
					"heatDiff: {:.3f} - unkUInt1: {:d} - unkUInt2: {:d} - incomingDamage: {:d} - baseDrainRate: {:.3f} - newDrainTimer: {:d}",
					heatDiff, unkUInt1, unkUInt2, incomingDamage, baseDrainRate, newDrainTimer
				);
			}

			const bool playerIsHoldingEnemy = playerStatus == 9;
			const bool enemyIsHoldingPlayer = playerStatus == 10;

			float expectedDiff = baseDrainRate;
			if (isDrunk) {
				expectedDiff *= 0.5f;
			}
			if (playerIsHoldingEnemy) {
				expectedDiff *= 0.5f;
			}
			else if (playerGotKnockedDown || enemyIsHoldingPlayer) {
				expectedDiff *= 10.0f;
			}

			if (incomingDamage == 0 && newHeatVal <= oldHeatVal && heatDiff != expectedDiff) {
				DbgBreak(); // Will sometimes be hit in between moves?
				spdlog::warn(
					"{:s} - oldHeatVal: {:.3f} - newHeatVal: {:.3f} - heatDiff: {:.3f} - retHeatVal: {:.3f} - incomingDamage: {:d} - baseDrainRate: {:.3f} - newDrainTimer: {:d}", 
					"GetNewHeatValue_5", oldHeatVal, newHeatVal, heatDiff, retHeatVal, incomingDamage, baseDrainRate, newDrainTimer
				);
			}

			utils::Log(5,
				"oldHeatVal: {:.3f} - newHeatVal: {:.3f} - heatDiff: {:.3f} - retHeatVal: {:.3f} - incomingDamage: {:d} - baseDrainRate: {:.3f} - newDrainTimer: {:d}",
				oldHeatVal, newHeatVal, heatDiff, retHeatVal, incomingDamage, baseDrainRate, newDrainTimer
			);
		}

		PlayerActor = playerActor; // save address of player actor for use in PatchedGetDisplayString()
		return retHeatVal;
	}
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

	if (CONFIG.UseOldPatch) {
		LegacyHeatFix::InitPatch();
	}
	else {
		{
			using namespace HeatFix;

			if (isDEBUG) {
				// Open debug logfile streams (not necessary but will save some time on the first real log message)
				utils::Log("", 1, "GetCurrentHeatValue");
				utils::Log("", 2, "GetMaxHeatValue");
				utils::Log("", 3, "GetNewHeatDrainTimer");
				utils::Log("", 4, "GetNewHeatValue_1");
				utils::Log("", 5, "GetNewHeatValue_2");
				utils::Log("", 6, "PatchedIsPlayerDrunk");
				utils::Log("", 7, "PatchedGetDisplayString");
				utils::Log("", 8, "AddHeatHoldFinisher");
				utils::Log("", 9, "AddSubtractHeatWrapper");

				// GetCurrentHeatValue - verify we're calling the correct function
				auto getCurHeatPattern = pattern("48 8b 81 10 15 00 00 c5 fa 10 40 08 c3");
				if (getCurHeatPattern.count_hint(1).size() == 1) {
					const auto match = getCurHeatPattern.get_one();
					verifyGetCurHeat = (GetActorFloatType)match.get<void>();
				}

				// GetMaxHeatValue - verify we're calling the correct function
				auto getMaxHeatPattern = pattern("48 8b 81 10 15 00 00 c5 fa 10 40 0c c3");
				if (getMaxHeatPattern.count_hint(1).size() == 1) {
					const auto match = getMaxHeatPattern.get_one();
					verifyGetMaxHeat = (GetActorFloatType)match.get<void>();
				}

				// IsPlayerDrunk - verify we're calling the correct function
				auto isPlayerDrunkPattern = pattern("48 8b 91 10 15 00 00 33 c0 66 83 7a 20 0a");
				if (isPlayerDrunkPattern.count_hint(1).size() == 1) {
					const auto match = isPlayerDrunkPattern.get_one();
					verifyIsPlayerDrunk = (GetActorBoolType)match.get<void>();
				}

				// Call to IsPlayerDrunk() in the block that's executed if (incoming damage > 0)
				// Just using this to check whether the Heat value changed before this point or not
				auto callIsPlayerDrunk = pattern("ff 90 90 02 00 00 85 c0 74 05 c4 c1 4a 59 f2 c4 c1");
				if (callIsPlayerDrunk.count_hint(1).size() == 1) {
					const auto match = callIsPlayerDrunk.get_one();
					const void *callAddr = match.get<void>();
					const void *retAddr = match.get<void>(6);
					Trampoline *trampoline = Trampoline::MakeTrampoline(callAddr);
					// Nop this instruction:
					// 0xff, 0x90, 0x90, 0x02, 0x00, 0x00 (CALL qword ptr [RAX + 0x290])
					Nop(callAddr, 6);

					// RSP is already properly aligned at this point since we replace a call with another call.
					// RCX contains the address of the player actor.
					const uint8_t payload[] = {
						0x48, 0x81, 0xec, 0x80, 0x00, 0x00, 0x00, // sub rsp, 0x80 (0x20 shadow space + 0x60 for XMM backups)
						0x0f, 0x29, 0x44, 0x24, 0x20, //  movaps XMMWORD PTR[rsp + 0x20],xmm0
						0x0f, 0x29, 0x4c, 0x24, 0x30, //  movaps XMMWORD PTR[rsp + 0x30],xmm1
						0x0f, 0x29, 0x54, 0x24, 0x40, //  movaps XMMWORD PTR[rsp + 0x40],xmm2
						0x0f, 0x29, 0x5c, 0x24, 0x50, //  movaps XMMWORD PTR[rsp + 0x50],xmm3
						0x0f, 0x29, 0x64, 0x24, 0x60, //  movaps XMMWORD PTR[rsp + 0x60],xmm4
						0x0f, 0x29, 0x6c, 0x24, 0x70, //  movaps XMMWORD PTR[rsp + 0x70],xmm5
						0x41, 0x0f, 0x28, 0xc9, // movaps xmm1, xmm9 (copy new Heat value to param2)
						0x41, 0x89, 0xf8, // mov r8d, edi (copy new Heat drain timer to param3)
						0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // movabs rax, PatchedIsPlayerDrunk
						0xff, 0xd0, // call rax
						0x0f, 0x28, 0x44, 0x24, 0x20, // movaps xmm0,XMMWORD PTR[rsp + 0x20]
						0x0f, 0x28, 0x4c, 0x24, 0x30, // movaps xmm1,XMMWORD PTR[rsp + 0x30]
						0x0f, 0x28, 0x54, 0x24, 0x40, // movaps xmm2,XMMWORD PTR[rsp + 0x40]
						0x0f, 0x28, 0x5c, 0x24, 0x50, // movaps xmm3,XMMWORD PTR[rsp + 0x50]
						0x0f, 0x28, 0x64, 0x24, 0x60, // movaps xmm4,XMMWORD PTR[rsp + 0x60]
						0x0f, 0x28, 0x6c, 0x24, 0x70, // movaps xmm5,XMMWORD PTR[rsp + 0x70]
						0x48, 0x81, 0xc4, 0x80, 0x00, 0x00, 0x00, // add rsp, 0x80 (restore rsp)
						0xe9, 0x00, 0x00, 0x00, 0x00, // jmp retAddr
					};

					std::byte *space = trampoline->RawSpace(sizeof(payload));
					memcpy(space, payload, sizeof(payload));

					const auto funcAddr = utils::GetFuncAddr(PatchedIsPlayerDrunk);
					// 7 + 5 + 5 + 5 + 5 + 5 + 5 + 4 + 3 + 2 = 46
					memcpy(space + 46, &funcAddr, sizeof(funcAddr));

					WriteOffsetValue(space + sizeof(payload) - 4, retAddr);
					InjectHook(callAddr, space, PATCH_JUMP);
				}

				// AddSubtractHeat - to verify we're calling the correct function
				auto addHeatPattern = pattern("48 89 5c 24 08 57 48 83 ec 30 48 8b 01 8b fa 48 8b d9 ff 90 20 03 00 00");
				if (addHeatPattern.count_hint(1).size() == 1) {
					const auto match = addHeatPattern.get_one();
					verifyAddHeat = (AddSubtractHeatType)match.get<void>();
				}


				// Don't know when any of these get hit. Hooking them just to be notified if/when they are called.
				// Of these calls only the 2nd one has any real potential to be buggy (since I don't know its charge amount).
				auto unkAddSubHeat_1 = pattern("ba 40 ed ff ff 48 8b cf ff 90 18 03 00 00"); // subtracts 4800 Heat
				auto unkAddSubHeat_2 = pattern("ff 90 18 03 00 00 c5 fc 10 45 80"); // unknown value
				auto unkAddSubHeat_3 = pattern("41 ff 90 18 03 00 00 b8 01 00 00 00"); // adds 100000, which would immediately max out Heat
				auto unkAddSubHeat_4 = pattern("49 8b c9 ff 90 18 03 00 00"); // subtracts 100000, which would immediately drain all Heat
				if (unkAddSubHeat_1.count_hint(1).size() == 1) {
					utils::Log("Found pattern: unkAddSubHeat_1");
					const auto match = unkAddSubHeat_1.get_one();
					const void *callAddr = match.get<void>(8);
					Nop(callAddr, 6);
					Trampoline *trampoline = Trampoline::MakeTrampoline(callAddr);
					InjectHook(callAddr, trampoline->Jump(AddSubtractHeatWrapper), PATCH_CALL);
				}
				if (unkAddSubHeat_2.count_hint(1).size() == 1) {
					utils::Log("Found pattern: unkAddSubHeat_2");
					const auto match = unkAddSubHeat_2.get_one();
					const void *callAddr = match.get<void>(0);
					Nop(callAddr, 6);
					Trampoline *trampoline = Trampoline::MakeTrampoline(callAddr);
					InjectHook(callAddr, trampoline->Jump(AddSubtractHeatWrapper), PATCH_CALL);
				}
				if (unkAddSubHeat_3.count_hint(1).size() == 1) {
					utils::Log("Found pattern: unkAddSubHeat_3");
					const auto match = unkAddSubHeat_3.get_one();
					const void *callAddr = match.get<void>(0);
					Nop(callAddr, 7);
					Trampoline *trampoline = Trampoline::MakeTrampoline(callAddr);
					InjectHook(callAddr, trampoline->Jump(AddSubtractHeatWrapper), PATCH_CALL);
				}
				if (unkAddSubHeat_4.count_hint(1).size() == 1) {
					utils::Log("Found pattern: unkAddSubHeat_4");
					const auto match = unkAddSubHeat_4.get_one();
					const void *callAddr = match.get<void>(3);
					Nop(callAddr, 6);
					Trampoline *trampoline = Trampoline::MakeTrampoline(callAddr);
					InjectHook(callAddr, trampoline->Jump(AddSubtractHeatWrapper), PATCH_CALL);
				}
			}


			/*
			* 66 83 ff 73 8d 6f 01 66 0f 43 ef 0f b7 fd
			* 
			* This is at the start of the section responsible for incrementing the Heat drain timer every time it gets reached.
			* There are a few more rules to it, but in general this is executed on every frame where all of these are true:
			* - Combat is ongoing (i.e. NOT paused by combat intro/outro or Heat moves)
			* - The player is alive and player's health is above 19%
			* - The player was NOT damaged by the enemy
			*/
			auto increaseDrainTimer = pattern("66 83 ff 73 8d 6f 01 66 0f 43 ef 0f b7 fd");
			if (increaseDrainTimer.count_hint(1).size() == 1) {
				utils::Log("Found pattern: HeatDrainTimer");
				const auto match = increaseDrainTimer.get_one();
				const void *patchAddr = match.get<void>(0);
				const void *retAddr = match.get<void>(14);
				Trampoline *trampoline = Trampoline::MakeTrampoline(patchAddr);
				/*
				* Nop these instructions:
				* 0x66, 0x83, 0xff, 0x73 (cmp di, 0x73)
				* 0x8d, 0x6f, 0x01 (lea ebp, [RDI + 0x1])
				* 0x66, 0x0f, 0x43, 0xef (cmovnc bp, di)
				* 0x0f, 0xb7, 0xfd (movzx edi, bp)
				*/
				Nop(patchAddr, 14);

				/*
				* - back up volatile registers
				* - make sure not to mess up the stack/backed up registers
				* - call our function that returns the new timer value
				* - restore registers/stack
				* - copy new timer value to EDI/EBP
				* - return to next instruction in UpdateHeat()
				*/
				const uint8_t payload[] = {
					0x50, // push rax
					0x51, // push rcx
					0x48, 0x89, 0xd9, // mov rcx, rbx (copy address of playerActor to param1)
					0x52, // push rdx
					0x89, 0xfa, // mov edx, edi (copy current timer to param2)
					0x41, 0x50, // push r8
					0x41, 0x51, // push r9
					0x41, 0x52, // push r10
					0x41, 0x53, // push r11
					0x48, 0x89, 0xe0, // mov rax, rsp (back up rsp before alignment)
					0x48, 0x83, 0xe4, 0xf0, // and rsp, -16 (align rsp to 16-byte boundary if necessary)
					0x50, // push rax (push old rsp contents onto the stack, which will unalign rsp again)
					0x48, 0x83, 0xec, 0x28, // sub rsp, 0x28 (0x20 shadow space for callee + 0x8 for stack alignment)
					0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // movabs rax, GetNewHeatDrainTimer
					0xff, 0xd0, // call rax
					0x48, 0x83, 0xc4, 0x28, // add rsp, 0x28 ("unreserve" shadow space + manual alignment)
					0x5c, // pop rsp (restore rsp from before 16-byte boundary alignment)
					0x41, 0x5b, // pop r11
					0x41, 0x5a, // pop r10
					0x41, 0x59, // pop r9
					0x41, 0x58, // pop r8
					0x5a, // pop rdx
					0x59, // pop rcx
					0x0f, 0xb7, 0xf8, // movzx edi, ax (copy new timer to EDI; will be written to memory towards the end of HeatUpdate)
					0x58, // pop rax
					0x89, 0xfd, // mov ebp, edi (copy new timer to EBP; will be checked right after the jmp below)
					0xe9, 0x00, 0x00, 0x00, 0x00, // jmp retAddr
				};

				std::byte *space = trampoline->RawSpace(sizeof(payload));
				memcpy(space, payload, sizeof(payload));

				const auto funcAddr = utils::GetFuncAddr(GetNewHeatDrainTimer);
				// 1 + 1 + 3 + 1 + 2 + 2 + 2 + 2 + 2 + 3 + 4 + 1 + 4 + 2 = 30
				memcpy(space + 30, &funcAddr, sizeof(funcAddr));

				WriteOffsetValue(space + sizeof(payload) - 4, retAddr);
				InjectHook(patchAddr, space, PATCH_JUMP);
			}


			/*
			* c5 32 58 0d 57 49 81 00 e9 - Heat boost by "Rescue Card"
			* c5 32 58 0d 4d 3c 81 00 e9 - Heat boost by "Phoenix Spirit"
			* 
			* Both of these automatically increase Heat while the player's health is at/below 19%.
			* Both have the same issue - they add a fixed amount of Heat every frame. So in Y3R
			* (which ALWAYS runs Heat calculations 60 times per second) they increase Heat at twice
			* their intended rate. Confirmed by comparison with PS3 version.
			*/
			auto addRescueCard = pattern("c5 32 58 0d 57 49 81 00 e9");
			auto addPhoenixSpirit = pattern("c5 32 58 0d 4d 3c 81 00 e9");
			if (addRescueCard.count_hint(1).size() == 1) {
				utils::Log("Found pattern: RescueCard");
				const auto match = addRescueCard.get_one();
				const void *addAddr = match.get<void>(0);
				const void *retAddr = match.get<void>(8);
				Trampoline *trampoline = Trampoline::MakeTrampoline(addAddr);
				// Nop this instruction after reading its offset value:
				// 0xc5, 0x32, 0x58, 0x0d, 0x57, 0x49, 0x81, 0x00 (vaddss xmm9,xmm9,DWORD PTR [rip+0x814957])
				float *pBoostValue;
				ReadOffsetValue(match.get<void>(4), pBoostValue); // read offset to original boostValue
				Nop(addAddr, 8);

				// XMM8 will always be 0.0f here, so we can use it as temporary storage and reset it to 0.0f after.
				// There are probably better ways of dividing the original boost value by 2 before adding it to XMM9.
				// But this method seems to work just fine.
				const uint8_t payload[] = {
					0x00, 0x00, 0x00, 0x00, // floatMulti
					0xf3, 0x44, 0x0f, 0x58, 0x05, 0x00, 0x00, 0x00, 0x00, // addss xmm8, dword ptr [pBoostValue]
					0xf3, 0x44, 0x0f, 0x59, 0x05, 0x00, 0x00, 0x00, 0x00, // mulss xmm8, dword ptr [floatMulti]
					0xf3, 0x45, 0x0f, 0x58, 0xc8, // addss xmm9, xmm8 (add lowered boostValue to XMM9)
					0x45, 0x0f, 0x57, 0xc0, // xorps xmm8,xmm8 (zero-out xmm8)
					0xe9, 0x00, 0x00, 0x00, 0x00, // jmp retAddr
				};

				std::byte *space = trampoline->RawSpace(sizeof(payload));
				memcpy(space, payload, sizeof(payload));

				memcpy(space, &floatMulti, sizeof(floatMulti)); // copy floatMulti to first 4 bytes of payload
				WriteOffsetValue(space + 4 + 5, pBoostValue); // write offset to original boost value
				WriteOffsetValue(space + 4 + 9 + 5, space); // write offset to copied floatMulti

				WriteOffsetValue(space + sizeof(payload) - 4, retAddr);
				InjectHook(addAddr, space + 4, PATCH_JUMP); // first instruction of payload starts at offset +4
			}
			if (addPhoenixSpirit.count_hint(1).size() == 1) {
				utils::Log("Found pattern: PhoenixSpirit");
				const auto match = addPhoenixSpirit.get_one();
				const void *addAddr = match.get<void>(0);
				const void *retAddr = match.get<void>(8);
				Trampoline *trampoline = Trampoline::MakeTrampoline(addAddr);
				// Nop this instruction after reading its offset value:
				// 0xc5, 0x32, 0x58, 0x0d, 0x4d, 0x3c, 0x81, 0x00 (vaddss xmm9,xmm9,DWORD PTR [rip+0x813c4d])
				float *pBoostValue;
				ReadOffsetValue(match.get<void>(4), pBoostValue); // read offset to original boostValue
				Nop(addAddr, 8);

				// XMM8 will always be 0.0f here, so we can use it as temporary storage and reset it to 0.0f after.
				const uint8_t payload[] = {
					0x00, 0x00, 0x00, 0x00, // floatMulti
					0xf3, 0x44, 0x0f, 0x58, 0x05, 0x00, 0x00, 0x00, 0x00, // addss xmm8, dword ptr [pBoostValue]
					0xf3, 0x44, 0x0f, 0x59, 0x05, 0x00, 0x00, 0x00, 0x00, // mulss xmm8, dword ptr [floatMulti]
					0xf3, 0x45, 0x0f, 0x58, 0xc8, // addss xmm9, xmm8 (add lowered boostValue to XMM9)
					0x45, 0x0f, 0x57, 0xc0, // xorps xmm8,xmm8 (zero-out xmm8)
					0xe9, 0x00, 0x00, 0x00, 0x00, // jmp retAddr
				};

				std::byte *space = trampoline->RawSpace(sizeof(payload));
				memcpy(space, payload, sizeof(payload));

				memcpy(space, &floatMulti, sizeof(floatMulti)); // copy floatMulti to first 4 bytes of payload
				WriteOffsetValue(space + 4 + 5, pBoostValue); // write offset to original boost value
				WriteOffsetValue(space + 4 + 9 + 5, space); // write offset to copied floatMulti

				WriteOffsetValue(space + sizeof(payload) - 4, retAddr);
				InjectHook(addAddr, space + 4, PATCH_JUMP); // first instruction of payload starts at offset +4
			}

			/*
			* 75 0d c4 c1 78 28 f8 0f b7 fe c4 41 32 58 cb - Heat boost by "True Lotus Flare Fist"
			* 
			* Keeps on charging Heat while the player charges up the "Lotus Flare Fist" attack.
			* Same issue as the the 2 above - charging rate in Y3R is twice of the intended/PS3 rate.
			*/
			auto addLotusFlare = pattern("75 0d c4 c1 78 28 f8 0f b7 fe c4 41 32 58 cb");
			if (addLotusFlare.count_hint(1).size() == 1) {
				utils::Log("Found pattern: TrueLotusFlareFist");
				const auto match = addLotusFlare.get_one();
				const void *addAddr = match.get<void>(10);
				const void *retAddr = match.get<void>(15);
				Trampoline *trampoline = Trampoline::MakeTrampoline(addAddr);
				// Nop this instruction:
				// 0xc4, 0x41, 0x32, 0x58, 0xcb (vaddss xmm9,xmm9,xmm11)
				Nop(addAddr, 5);

				// XMM11 will contain the original boost value (which we want to halve).
				const uint8_t payload[] = {
					0x00, 0x00, 0x00, 0x00, // floatMulti
					0xf3, 0x44, 0x0f, 0x59, 0x1d, 0x00, 0x00, 0x00, 0x00, // mulss xmm11, dword ptr [floatMulti]
					0xf3, 0x45, 0x0f, 0x58, 0xcb, // addss xmm9, xmm11 (add lowered boostValue to XMM9)
					0xe9, 0x00, 0x00, 0x00, 0x00, // jmp retAddr
				};

				std::byte *space = trampoline->RawSpace(sizeof(payload));
				memcpy(space, payload, sizeof(payload));

				memcpy(space, &floatMulti, sizeof(floatMulti)); // copy floatMulti to first 4 bytes of payload
				WriteOffsetValue(space + 4 + 5, space); // write offset to copied floatMulti

				WriteOffsetValue(space + sizeof(payload) - 4, retAddr);
				InjectHook(addAddr, space + 4, PATCH_JUMP); // first instruction of payload starts at offset +4
			}

			/*
			* ba 0a 00 00 00 48 8b cb ff 90 18 03 00 00 80 bb - Heat boost by "Golden Dragon Spirit"
			* ba 0a 00 00 00 48 8b cb ff 90 18 03 00 00 85 c0 - Heat boost by "Leech Gloves"
			* 
			* Both call the AddSubtractHeat(void **actor, int32_t chargeAmount) function to continuously
			* add Heat on every frame while the ability is active. So while holding down the taunt button
			* for "Golden Dragon Spirit" or while holding onto an enemy for the "Leech Gloves".
			* Again - I verified these run at twice their intended rate by comparing against the PS3 version.
			* 
			* AddSubtractHeat() is entirely separate from UpdateHeat() and is used by various actions that
			* modify the player's Heat value. Other examples that use it would be the start of a player
			* Heat move draining a big chunk of Heat at once or using a Heat restoring item (e.g. Staminan).
			* Calls to this function always look similar to this:
			* 
			* MOV  RAX,qword ptr [PlayerActor] (get address to player's vfTable)
			* ... (move param1/param2 into RCX/RDX)
			* CALL qword ptr [RAX + 0x318] (call function at offset x inside vfTable)
			* 
			* param2 is indeed an int32_t and not a float like one would expect (since Heat is stored as a float).
			* Internally this function will convert param2 to a float, check whether the requested amount can be
			* added/subtracted to/from the current Heat value, execute the addition/subtraction and then return
			* either 1 (if successful) or 0 (add/subtract failed; mostly when Heat is already full/empty).
			* 
			* Both of these use a hardcoded immediate value for the chargeAmount, so we can simply divide the
			* original immediate value (i.e. 10 in both cases) by 2 and call it a day.
			*/
			auto addGoldenDragonSpirit = pattern("ba 0a 00 00 00 48 8b cb ff 90 18 03 00 00 80 bb");
			auto addLeechGloves = pattern("ba 0a 00 00 00 48 8b cb ff 90 18 03 00 00 85 c0");
			if (addGoldenDragonSpirit.count_hint(1).size() == 1) {
				utils::Log("Found pattern: GoldenDragonSpirit");
				const auto match = addGoldenDragonSpirit.get_one();
				void *pImmediate = match.get<void>(1); // immediate value for mov edx, immediate
				uint32_t origValue;
				memcpy(&origValue, pImmediate, sizeof(origValue));
				const uint32_t newValue = origValue / integerDiv;
				memcpy(pImmediate, &newValue, sizeof(newValue));
			}
			if (addLeechGloves.count_hint(1).size() == 1) {
				utils::Log("Found pattern: LeechGloves");
				const auto match = addLeechGloves.get_one();
				void *pImmediate = match.get<void>(1); // immediate value for mov edx, immediate
				uint32_t origValue;
				memcpy(&origValue, pImmediate, sizeof(origValue));
				const uint32_t newValue = origValue / integerDiv;
				memcpy(pImmediate, &newValue, sizeof(newValue));
			}

			/*
			* Call to AddSubtractHeat() when holding down some combo finishers 
			* (e.g. 3xLight hit + holding down Heavy hit / same for 4xLight hit)
			* 
			* If the final hit of one of these combos damages the enemy, the player can keep holding down the
			* finisher button (i.e. Triangle for PS / Y for XBox) to continuously charge Heat for up to 2 seconds.
			* This one is pretty interesting because it is the only case of continuous Heat increase that the
			* Y3R devs actually tried to fix.
			* 
			* In the PS3 version the maximum charge duration is 60 frames (i.e. 2 seconds at 30 fps).
			* In Y3R they doubled this to 120 frames in order to account for the new 60 fps framerate.
			* So no issues as far as the charge duration goes. Again - keep in mind that Y3R ALWAYS runs
			* its logic 60 times per second, even if the frame limit is set to 30 fps!
			* 
			* In the PS3 version the charge amount per frame is 17.0f, so the correct charge amount for Y3R would
			* have been 8.5f per frame. Main issue is that AddSubtractHeat() expects its amount parameter as an int32_t.
			* Meaning the charge amount will be truncated to just 8 per second when Y3R converts it to an integer.
			* 
			* Curiously the float that gets converted to an integer right before the call to AddSubtractHeat() in Y3R is
			* not 8.5f but rather 8.3333f. I don't know why that is the case. Maybe the PS3 version's value was 16.6666f
			* and it got rounded up to 17 before calling its version of AddSubtractHeat()? Or maybe the PS3 version's
			* value is actually 17.0f from the get go and the Y3R devs either failed to correctly divide by 2 (unlikely)
			* or intentionally changed the value from 8.5f to 8.3333f. Though to say for sure without reverse engineering
			* the PS3 version's binary.
			* 
			* What I do know is that the base value which is used for the calculation that results in Y3R's 8.3333f value
			* seems to originate from somewhere inside the game's "motion/property.bin" file. Or at least that's what I thought.
			* But "property.bin" and "ActionSet.cas" are literally 100% IDENTICAL betweeen Y3 PS3 and (unmodified) Y3R. So maybe
			* the difference comes from somewhere else entirely? It's just that the "Yakuza 3 Rebalanced" mod modifies exactly
			* these 2 files and it does indeed change the 8.3333f value for 3xLight hit + Finisher to 10.0f.
			* But either way - that's besides the point as far as this call to AddSubtractHeat() goes.
			* 
			* Fix is pretty simple - call AddSubtractHeat() just like the game would do on its own and "manually" add the
			* truncated/missing 0.5f to the current Heat value after AddSubtractHeat() is done.
			*/
			auto callAddHeat = pattern("c5 fa 2d d1 48 8b cf ff 90 18 03 00 00");
			if (callAddHeat.count_hint(1).size() == 1) {
				utils::Log("Found pattern: HoldComboFinisher");
				const auto match = callAddHeat.get_one();
				const void *callAddr = match.get<void>(7);
				Nop(callAddr, 6);
				Trampoline *trampoline = Trampoline::MakeTrampoline(callAddr);
				InjectHook(callAddr, trampoline->Jump(AddHeatHoldFinisher), PATCH_CALL);
			}


			/*
			* c5 e2 5f f2 ff 90 40 03 00 00
			* 
			* Call to GetMaxHeatValue() near the end of UpdateHeat()
			* The new/calculated Heat value is finalized (except for bounds-checking) before this call is reached.
			* So this is a good spot to inject our own function and modify the new Heat value right before it gets
			* written to memory.
			*/
			auto finalHeatCalc = pattern("c5 e2 5f f2 ff 90 40 03 00 00");
			if (finalHeatCalc.count_hint(1).size() == 1) {
				utils::Log("Found pattern: FinalHeatCalc");
				auto match = finalHeatCalc.get_one();
				void *callAddr = match.get<void>(4);
				void *retAddr = match.get<void>(10);
				Trampoline *trampoline = Trampoline::MakeTrampoline(callAddr);
				// Nop this instruction:
				// 0xff, 0x90, 0x40, 0x03, 0x00, 0x00 (CALL qword ptr [RAX + 0x340])
				Nop(callAddr, 6);

				/*
				* RBX/RCX contain the address of the player actor.
				* XMM3 contains the calculated Heat value (before it is checked against 0.0f and MAX Heat boundaries)
				* XMM6 contains max(XMM3,0.0f) (i.e. calculated Heat value checked against 0.0f)
				* 
				* It is important that we use XMM3 and NOT XMM6 to calculate the difference between the last frame's
				* Heat value and the new value. Otherwise we wouldn't be able to tell the actual difference once we
				* get closer to 0. I made this mistake in an earlier version, with the result that my modified Heat
				* value only ever got closer and closer to 0 without ever reaching it (until a rounding error eventually
				* makes it 0, but that's besides the point).
				*/
				const uint8_t payload[] = {
					0x0f, 0x28, 0xcb, // movaps xmm1, xmm3 (copy new/calculated Heat value to param2)
					0x0f, 0x28, 0xd7, // movaps xmm2, xmm7 (copy baseDrainRate to param3)
					0x41, 0x89, 0xf9, // mov r9d, edi (copy new Heat drain timer to param4)
					0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // movabs rax, GetNewHeatValue
					0xff, 0xd0, // call rax
					0x0f, 0x28, 0xf0, // movaps xmm6, xmm0 (copy new/returned Heat value to XMM6)
					0x48, 0x89, 0xd9, // mov rcx, rbx (copy address of playerActor to param1)
					0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // movabs rax, GetMaxHeatValue
					0xff, 0xd0, // call rax
					0xe9, 0x00, 0x00, 0x00, 0x00 // jmp retAddr
				};
				/*
				* The game expects the player's maximum Heat value to be inside XMM0 and the new/calculated Heat
				* value inside XMM6 after we return to HeatUpdate(). 
				* The new Heat drain timer should be inside (E)DI (which it already is since we don't change it here).
				* 
				* Shortly after we return to HeatUpdate(), the game will do a few final checks on these values
				* (e.g. making sure that the new Heat value doesn't exceed the maximum Heat value) and will then
				* write them to memory.
				*/

				std::byte *space = trampoline->RawSpace(sizeof(payload));
				memcpy(space, payload, sizeof(payload));

				auto pGetNewHeatValue = utils::GetFuncAddr(GetNewHeatValue);
				// 3 + 3 + 3 + 2 = 11
				memcpy(space + 11, &pGetNewHeatValue, sizeof(pGetNewHeatValue));

				auto pGetMaxHeatValue = utils::GetFuncAddr(GetMaxHeatValue);
				// 5 + 2 + 8 = 15
				memcpy(space + sizeof(payload) - 15, &pGetMaxHeatValue, sizeof(pGetMaxHeatValue));

				WriteOffsetValue(space + sizeof(payload) - 4, retAddr);
				InjectHook(callAddr, space, PATCH_JUMP);
			}
		}
	}

	if (CONFIG.ShowHeatValues) {
		using namespace HeatFix;

		/*
		* e8 ?? ?? ?? ?? 4c 8b f8 48 8b ac 24 98
		*
		* Call to function that returns a string (char *). Returned string is then rendered to the screen.
		* This call in particular seems to fetch the DisplayString of the (targeted) enemy.
		*/
		auto callGetDisplayString = pattern("e8 ? ? ? ? 4c 8b f8 48 8b ac 24 98");
		if (callGetDisplayString.count_hint(1).size() == 1) {
			utils::Log("Found pattern: GetDisplayString");
			const auto match = callGetDisplayString.get_one();
			Trampoline *trampoline = Trampoline::MakeTrampoline(match.get<void>());
			ReadCall(match.get<void>(), origGetDisplayString);
			InjectHook(match.get<void>(), trampoline->Jump(PatchedGetDisplayString), PATCH_CALL);
		}
	}

	// log current time to file to get some feedback once hook is done
	Yakuza3::Done("Hook done!");
}
