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
	static uint64_t dbg_Counter1 = 0, dbg_Counter2 = 0, dbg_Counter3 = 0;
	static uint8_t drainTimeLimiter = 1;
	static uint16_t lastDrainTimer = 0, substituteTimer = 0;
	static constexpr uint8_t drainTimeMulti = 2;
	static constexpr uint16_t MAX_DrainTimer = 0x73;

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
		uint8_t oldDrainTimeLimiter;
		if (curDrainTimer != lastDrainTimer || curDrainTimer == 0) {
			if (s_Debug) {
				if (curDrainTimer != 0) {
					DebugBreak(); // should never happen
					utils::Log("");
				}
			}

			// drain timer got reset somewhere else
			substituteTimer = 0;
			drainTimeLimiter = 1;
			oldDrainTimeLimiter = drainTimeLimiter; // for logging purposes
			// Immediately increase to 1, so we'll be able to notice if the timer gets reset.
			// Otherwise we wouldn't be able to tell whether we kept the timer at 0 intentionally
			// or if it was reset to 0 externally. (Could lead to starting Heat drain too early)
			newDrainTimer = 1;
		}
		else if (curDrainTimer == 1) {
			// keep "real" drain timer at 1 for (drainTimeMulti + 1) frames,
			// to make up for immediately increasing it from 0 to 1
			newDrainTimer = 1;
			oldDrainTimeLimiter = drainTimeLimiter; // for logging purposes
			if (substituteTimer == drainTimeMulti) {
				newDrainTimer++;
			}
			else {
				substituteTimer++;
			}

			if (s_Debug) {
				if (substituteTimer > drainTimeMulti || drainTimeLimiter != 1) {
					DebugBreak(); // should never happen
					utils::Log("");
				}
			}
		}
		else {
			if (s_Debug) {
				if (substituteTimer != drainTimeMulti) {
					DebugBreak(); // should never happen
					utils::Log("");
				}
			}

			// increase drain timer every (drainTimeMulti) frames
			newDrainTimer = curDrainTimer;
			oldDrainTimeLimiter = drainTimeLimiter; // for logging purposes
			if (drainTimeLimiter == drainTimeMulti) {
				drainTimeLimiter = 1;
				if (newDrainTimer < MAX_DrainTimer) {
					newDrainTimer++;
				}
			}
			else {
				drainTimeLimiter++;
			}

			if (s_Debug) {
				if (drainTimeLimiter > drainTimeMulti || newDrainTimer > MAX_DrainTimer) {
					DebugBreak(); // should never happen
					utils::Log("");
				}
			}
		}

		if (s_Debug) {
			utils::Log(format(
				"{:s} - {:d} - TzNow: {:s} - playerActor: {:p} - drainTimeMulti: {:d} - drainTimeLimiter: {:d} - curDrainTimer: {:d} - substituteTimer: {:d} - newDrainTimer: {:d}", 
				"GetNewHeatDrainTimer", dbg_Counter3++, utils::TzString_ms(), (void *)playerActor, drainTimeMulti, oldDrainTimeLimiter, curDrainTimer, substituteTimer, newDrainTimer), 3
			);
		}
		lastDrainTimer = newDrainTimer;
		return newDrainTimer;
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
			* 66 83 ff 73 8d 6f 01 66 0f 43 ef 0f b7 fd
			*/
			auto increaseDrainTimer = pattern("66 83 ff 73 8d 6f 01 66 0f 43 ef 0f b7 fd");
			if (increaseDrainTimer.count_hint(1).size() == 1) {
				utils::Log("Found pattern: IncreaseDrainTimer");
				auto match = increaseDrainTimer.get_one();
				void *patchAddr = match.get<void>(0);
				void *retAddr = match.get<void>(14);
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
				* - call function that returns new timer value
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

				auto funcAddr = utils::GetFuncAddr(GetNewHeatDrainTimer);
				// 1 + 1 + 3 + 1 + 2 + 2 + 2 + 2 + 2 + 3 + 4 + 1 + 4 + 2 = 30
				memcpy(space + 30, &funcAddr, sizeof(funcAddr));

				WriteOffsetValue(space + sizeof(payload) - 4, retAddr);
				InjectHook(patchAddr, space, PATCH_JUMP);
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

				auto pGetMaxHeatValue = utils::GetFuncAddr(GetMaxHeatValue);
				// 10 + 2 + 3 + 2 = 17
				memcpy(space + 17, &pGetMaxHeatValue, sizeof(pGetMaxHeatValue));

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
