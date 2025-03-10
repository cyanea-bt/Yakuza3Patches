#include "utils.h"
#include "Yakuza3.h"
#include "Yakuza3HeatFix.h"


namespace HeatFix {
	using namespace std;
	static uint8_t drainTimeLimiter = 1;
	static uint16_t lastDrainTimer = 0, substituteTimer = 0;

	GetActorBoolType verifyIsPlayerDrunk = nullptr;
	GetActorFloatType verifyGetCurHeat = nullptr, verifyGetMaxHeat = nullptr;
	AddSubtractHeatType verifyAddHeat = nullptr;

	// for debugging purposes
	uint8_t AddSubtractHeatWrapper(void **playerActor, int32_t amount) {
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
		const uint8_t result = AddHeat(playerActor, amount);

		if (isDEBUG) {
			const float maxHeat = GetMaxHeat(playerActor);
			const float heatAfter = GetCurHeat(playerActor);
			const float heatDiff = heatAfter - heatBefore;
			utils::Log(9,
				"amount: {:d} - result: {:d} - maxHeat: {:.3f} - heatBefore: {:.3f} - heatAfter: {:.3f} - heatDiff: {:.3f}",
				amount, result, maxHeat, heatBefore, heatAfter, heatDiff
			);

			DebugBreak();
			utils::Log("");
			winutils::ShowErrorMessage("AddSubtractHeatWrapper"); // In case I don't have a debugger attached
		}

		return result;
	}

	uint8_t AddHeatComboFinisher(void **playerActor, int32_t amount) {
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

	float GetMaxHeatValue(void **playerActor) {
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

	bool PatchedIsPlayerDrunk(void **playerActor, const float newHeatVal, const uint16_t newDrainTimer) {
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


	GetDisplayStringType origGetDisplayString = nullptr;
	static string replaceStr("Cool replacement string");

	char* PatchedGetDisplayString(void *param1, uint32_t param2, uint32_t param3) {
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


	uint16_t GetNewHeatDrainTimer(void **playerActor, const uint16_t curDrainTimer) {
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

	float GetNewHeatValue(void **playerActor, const float newHeatVal, const float baseDrainRate, const uint16_t newDrainTimer) {
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
		if (CONFIG.FixHeatDrain && newHeatVal < oldHeatVal) {
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
