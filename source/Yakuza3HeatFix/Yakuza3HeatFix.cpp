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
	static uint8_t drainLimiter = 1; 
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
		float curHeat = getCurHeat(playerActor);

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
		float maxHeat = getMaxHeat(playerActor);

		if (s_Debug) {
			utils::Log(format("{:s} - {:d} - TzNow: {:s} - playerActor: {:p} - vfTable: {:p} - getMaxHeat: {:p} - maxHeat: {:f}",
				"GetMaxHeatValue", dbg_Counter2++, utils::TzString_ms(), (void *)playerActor, (void *)vfTable, (void *)getMaxHeat, maxHeat), 2);
		}
		return maxHeat;
	}

	static uint16_t GetNewHeatDrainTimer(void **playerActor, const uint16_t heatDrainTimer) {
		// MOVZX EDI,word ptr [param_1 + 0x14c8]
		const uint16_t oldHeatDrainTimer = *(uint16_t *)((uintptr_t)playerActor + 0x14c8);
		
		/*
		uint16_t newHeatDrainTimer = 0;
		if (heatDrainTimer == (oldHeatDrainTimer + 1)) {
			if (drainLimiter == drainTimeMulti) {
				newHeatDrainTimer = heatDrainTimer;
				drainLimiter = 1;
			}
			else {
				newHeatDrainTimer = oldHeatDrainTimer;
				drainLimiter++;
			}
		}
		else {
			newHeatDrainTimer = heatDrainTimer;
			drainLimiter = 0;
		}
		*/

		uint16_t newHeatDrainTimer = heatDrainTimer;
		const uint8_t oldDrainLimiter = drainLimiter; // for logging purposes
		if (heatDrainTimer == (oldHeatDrainTimer + 1) && heatDrainTimer != MAX_DrainTimer) {
			if (drainLimiter == drainTimeMulti) {
				drainLimiter = 1;
			}
			else {
				newHeatDrainTimer = oldHeatDrainTimer;
				drainLimiter++;
			}
		}
		else {
			drainLimiter = 1;
		}

		if (s_Debug) {
			utils::Log(format(
				"{:s} - {:d} - TzNow: {:s} - playerActor: {:p} - drainTimeMulti: {:d} - drainLimiter: {:d} - oldHeatDrainTimer: {:d} - heatDrainTimer: {:d} - newHeatDrainTimer: {:d}", 
				"GetNewHeatDrainTimer", dbg_Counter3++, utils::TzString_ms(), (void *)playerActor, drainTimeMulti, oldDrainLimiter, oldHeatDrainTimer, heatDrainTimer, newHeatDrainTimer), 3
			);
		}
		return newHeatDrainTimer;
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
