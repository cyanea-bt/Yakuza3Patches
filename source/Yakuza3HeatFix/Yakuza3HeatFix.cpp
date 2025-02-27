#define WIN32_LEAN_AND_MEAN
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
#include "Yakuza3HeatFix.h"

#if _DEBUG
static constexpr bool s_Debug = true;
#else
static constexpr bool s_Debug = false;
#endif // _DEBUG
static config::Config s_Config;


namespace HeatFix {
	using namespace std;
	static uint64_t dbg_Counter1 = 0, dbg_Counter2 = 0, dbg_Counter3 = 0, dbg_Counter4 = 0, dbg_Counter5 = 0;
	static uint8_t drainTimeLimiter = 1;
	static uint16_t lastDrainTimer = 0, substituteTimer = 0;
	static constexpr uint16_t MAX_DrainTimer = 0x73;

	typedef bool(*GetActorBoolType)(void **);
	typedef float(*GetActorFloatType)(void **);
	static GetActorFloatType verifyGetCurHeat = nullptr, verifyGetMaxHeat = nullptr;
	static constexpr float floatMulti = 0.5f; // for float values that have to be halved when running at 60 fps

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
		const float curHeat = getCurHeat(playerActor);

		if (s_Debug) {
			utils::Log(format("{:s} - {:d} - TzNow: {:s} - playerActor: {:p} - vfTable: {:p} - getCurHeat: {:p} - curHeat: {:f}",
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
		const float maxHeat = getMaxHeat(playerActor);

		if (s_Debug) {
			utils::Log(format("{:s} - {:d} - TzNow: {:s} - playerActor: {:p} - vfTable: {:p} - getMaxHeat: {:p} - maxHeat: {:f}",
				"GetMaxHeatValue", dbg_Counter2++, utils::TzString_ms(), (void *)playerActor, (void *)vfTable, (void *)getMaxHeat, maxHeat), 2);
		}
		return maxHeat;
	}

	static uint16_t GetNewHeatDrainTimer(void **playerActor, const uint16_t curDrainTimer) {
		uint16_t newDrainTimer;
		const uint16_t MAX_SubstituteTimer = (s_Config.DrainTimeMulti * 2 - 2); // -2 because timer starts at 0
		uint8_t oldDrainTimeLimiter; // for logging purposes
		uint16_t oldSubstituteTimer; // for logging purposes
		if (s_Debug) {
			oldDrainTimeLimiter = drainTimeLimiter;
			oldSubstituteTimer = substituteTimer;

			// movzx eax,word ptr [param_1 + 0x1abc]
			const uint16_t incomingDamage = *(uint16_t *)((uintptr_t)playerActor + 0x1abc);
			if (incomingDamage > 0) {
				DebugBreak(); // Drain timer shouldn't change on a frame with incoming damage.
				utils::Log("");
			}
		}

		if (s_Config.DisableHeatDrain) {
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

			if (s_Debug) {
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

			if (s_Debug) {
				if (substituteTimer > MAX_SubstituteTimer || drainTimeLimiter != 1) {
					DebugBreak(); // should never happen
					utils::Log("");
				}
			}
		}
		else {
			if (s_Debug) {
				if (substituteTimer != MAX_SubstituteTimer || substituteTimer == 0) {
					DebugBreak(); // should never happen
					utils::Log("");
				}
			}

			// Increase drain timer every (drainTimeMulti) frames
			newDrainTimer = curDrainTimer;
			if (drainTimeLimiter == s_Config.DrainTimeMulti) {
				drainTimeLimiter = 1;
				if (newDrainTimer < MAX_DrainTimer) {
					newDrainTimer++;
				}
			}
			else {
				drainTimeLimiter++;
			}

			if (s_Debug) {
				if (drainTimeLimiter > s_Config.DrainTimeMulti || newDrainTimer > MAX_DrainTimer) {
					DebugBreak(); // should never happen
					utils::Log("");
				}
			}
		}

		if (s_Debug) {
			utils::Log(format(
				"{:s} - {:d} - TzNow: {:s} - playerActor: {:p} - drainTimeMulti: {:d} - drainTimeLimiter: {:d} - curDrainTimer: {:d} - newDrainTimer: {:d} - substituteTimer: {:d}", 
				"GetNewHeatDrainTimer", dbg_Counter3++, utils::TzString_ms(), (void *)playerActor, s_Config.DrainTimeMulti, oldDrainTimeLimiter, curDrainTimer, newDrainTimer, oldSubstituteTimer), 3
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

		// MOV RAX,qword ptr [param_1]
		// CALL qword ptr [RAX + 0x290]
		uintptr_t *vfTable = (uintptr_t *)*playerActor;
		GetActorBoolType IsPlayerDrunk = (GetActorBoolType)vfTable[0x290 / sizeof(uintptr_t)];
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
			utils::Log(""); break;
		}

		if (s_Debug) {
			/*
			if (unkXMM2 != 0.0f) {
				DebugBreak();
				utils::Log(""); // XMM2 seems to always be 0.0f
			}
			*/

			// CMP  byte ptr [RBX + 0x1a49],0x3
			// JNZ +0x0e
			// CMP  byte ptr [RBX + 0x1a48],0x3c
			// JA  +0x05
			const uint8_t unkUInt1 = *(uint8_t *)((uintptr_t)playerActor + 0x1a49);
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
			else if (unkUInt1 != 0 && unkUInt1 != 2 && unkUInt1 != 3) {
				DebugBreak();
				utils::Log("");
			}
			const uint8_t unkUInt2 = *(uint8_t *)((uintptr_t)playerActor + 0x1a48);
			if (unkUInt2 > 0x3c) {
				DebugBreak(); // never happens?
				utils::Log("");
			}
			const bool playerGotKnockedDown = (playerStatus == 4) && (unkUInt1 == 0x3) && (unkUInt2 <= 0x3c);
			if (playerStatus == 4) {
				utils::Log(format(
					"{:s} - {:d} - TzNow: {:s} - heatDiff: {:f} - unkUInt1: {:d} - unkUInt2: {:d} - incomingDamage: {:d} - baseDrainRate: {:f} - newDrainTimer: {:d}",
					"GetNewHeatValue", dbg_Counter5++, utils::TzString_ms(), heatDiff, unkUInt1, unkUInt2, incomingDamage, baseDrainRate, newDrainTimer), 5
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
				DebugBreak();
 				utils::Log("");
			}

			utils::Log(format(
				"{:s} - {:d} - TzNow: {:s} - playerActor: {:p} - oldHeatVal: {:f} - newHeatVal: {:f} - heatDiff: {:f} - incomingDamage: {:d} - baseDrainRate: {:f} - newDrainTimer: {:d}",
				"GetNewHeatValue", dbg_Counter4++, utils::TzString_ms(), (void *)playerActor, oldHeatVal, newHeatVal, heatDiff, incomingDamage, baseDrainRate, newDrainTimer), 4
			);
		}
		//return GetCurrentHeatValue(playerActor); // Heat won't change with regular hits
		return newHeatVal;
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
		LegacyHeatFix::InitPatch();
	}
	else {
		{
			using namespace HeatFix;

			if (s_Debug) {
				// Open debug logfile streams (not necessary but will save some time on the first real log message)
				utils::Log("", 1);
				utils::Log("", 2);
				utils::Log("", 3);
				utils::Log("", 4);
				utils::Log("", 5);

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
			}

			/*
			* 66 83 ff 73 8d 6f 01 66 0f 43 ef 0f b7 fd
			* 
			* This is the section responsible for incrementing the Heat drain timer every time it gets reached.
			* There are a few more rules to it, but in general this is executed on every frame where all of these are true:
			* - Combat is ongoing (i.e. NOT paused by combat intro/outro or Heat moves)
			* - The player is alive and player's health is above 19%
			* - The player did NOT hit the enemy
			* - The player was NOT hit the enemy
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

				// XMM11 will contain the original boost value.
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
					0x0f, 0x57, 0xc9, // xorps xmm1, xmm1 (zero-out xmm1, for testing/debugging purposes)
					0x0f, 0x28, 0xce, // movaps xmm1, xmm6 (copy calculated Heat value to param2)
					0x0f, 0x28, 0xd7, // movaps xmm2, xmm7 (copy baseDrainRate to param3)
					0x41, 0x89, 0xf9, // mov r9d, edi (copy new Heat drain timer to param4)
					0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // movabs rax, GetNewHeatValue
					0xff, 0xd0, // call rax
					0x0f, 0x28, 0xf0, // movaps xmm6, xmm0 (copy returned/new Heat value to XMM6)
					0x48, 0x89, 0xd9, // mov rcx, rbx (copy address of playerActor to param1)
					0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // movabs rax, GetMaxHeatValue
					0xff, 0xd0, // call rax
					0xe9, 0x00, 0x00, 0x00, 0x00 // jmp retAddr
				};
				/*
				* The game expects the player's maximum Heat value to be inside XMM0 and the new/calculated Heat
				* value inside XMM6 after we return to HeatUpdate()!
				* 
				* Shortly after we return to the game's own code, the contents of (E)DI will be written to memory
				* as the new value for HeatUpdate()'s drain timer. This is the write instruction:
				* MOV word ptr [RBX + 0x14c8],DI
				*/

				std::byte *space = trampoline->RawSpace(sizeof(payload));
				memcpy(space, payload, sizeof(payload));

				auto pGetNewHeatValue = utils::GetFuncAddr(GetNewHeatValue);
				// 3 + 3 + 3 + 3 + 2 = 14
				memcpy(space + 14, &pGetNewHeatValue, sizeof(pGetNewHeatValue));

				auto pGetMaxHeatValue = utils::GetFuncAddr(GetMaxHeatValue);
				// 5 + 2 + 8 = 15
				memcpy(space + sizeof(payload) - 15, &pGetMaxHeatValue, sizeof(pGetMaxHeatValue));

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
