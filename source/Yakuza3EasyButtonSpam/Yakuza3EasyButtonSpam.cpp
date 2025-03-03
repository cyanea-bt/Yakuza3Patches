#include "utils.h"
#include "Yakuza3.h"


namespace EasySpam {
	using namespace std;
	static uint64_t dbg_Counter1 = 0, dbg_Counter2 = 0, dbg_Counter3 = 0, dbg_Counter4 = 0;

	typedef uint8_t (*GetEnemyThrowResistanceType)(uintptr_t);
	static GetEnemyThrowResistanceType enemyThrowResFunc = nullptr;

	typedef uint8_t (*AddHeatType)(uintptr_t, int32_t);
	static AddHeatType addHeatFunc = nullptr;

	// param1 here is the address of the player actor object
	static uint8_t PatchedChargeFeelTheHeat(uintptr_t param1, int32_t oldChargeAmount) {
		// CALL qword ptr [param_1 + 0x318]
		uintptr_t pAddHeat = *(uintptr_t *)(param1);
		pAddHeat = *(uintptr_t *)(pAddHeat + 0x318);
		const AddHeatType addHeat = (AddHeatType)pAddHeat;
		if (isDEBUG) {
			if (addHeat != addHeatFunc) {
				DebugBreak(); // verify we're calling the correct function
			}
		}

		int32_t newChargeAmount = 32000; // just need some sensible upper limit (MAX Heat in Chapter 1 seems to be 8000)
		if ((oldChargeAmount * CONFIG.ChargeFeelTheHeat) < newChargeAmount) {
			newChargeAmount = oldChargeAmount * CONFIG.ChargeFeelTheHeat;
		}
		/*
		* As far as I can tell - result should always be == 1, unless:
		* - newChargeAmount > 0 && current Heat == MAX Heat (Trying to add Heat, but Heat is already full)
		* - newChargeAmount < 0 && current Heat == 0.0f (Trying to subtract Heat, but Heat is already empty)
		*/
		const uint8_t result = addHeat(param1, newChargeAmount);

		if (isDEBUG) {
			utils::Log(fmt::format("ChargeFeelTheHeat - TzNow: {:s} - {:d} - oldChargeAmount: {:d} - FeelTheHeatChargeMulti: {:d} - newChargeAmount: {:d} - result: {:d}",
				utils::TzString_ms(), dbg_Counter4++, oldChargeAmount, CONFIG.ChargeFeelTheHeat, newChargeAmount, result), 4);
		}

		return result;
	}

	static uint8_t PatchedDecHoldPower(uint8_t oldHoldPower) {
		uint8_t newHoldPower = 0;
		if (oldHoldPower > CONFIG.EscapeEnemyGrab) {
			newHoldPower = oldHoldPower - CONFIG.EscapeEnemyGrab;
		}

		if (isDEBUG) {
			utils::Log(fmt::format("DecreaseHoldPower - TzNow: {:s} - {:d} - oldHoldPower: {:d} - EnemyHoldPowerSub: {:d} - newHoldPower: {:d}",
				utils::TzString_ms(), dbg_Counter3++, oldHoldPower, CONFIG.EscapeEnemyGrab, newHoldPower), 3);
		}

		return newHoldPower;
	}

	static uint8_t PatchedIncThrowResistance(uint8_t oldThrowRes) {
		uint8_t newThrowRes = UINT8_MAX;
		if (oldThrowRes + CONFIG.EnemyThrowResIncrease < newThrowRes) {
			newThrowRes = oldThrowRes + CONFIG.EnemyThrowResIncrease;
		}

		if (isDEBUG) {
			utils::Log(fmt::format("IncreaseThrowResistance - TzNow: {:s} - {:d} - oldThrowRes: {:d} - EnemyThrowResInc: {:d} - newThrowRes: {:d}",
				utils::TzString_ms(), dbg_Counter2++, oldThrowRes, CONFIG.EnemyThrowResIncrease, newThrowRes), 2);
		}

		return newThrowRes;
	}

	// param1 here points to the actor object of the enemy that the player is trying to throw
	static uint8_t PatchedGetEnemyThrowResistance(uintptr_t param1) {
		// CALL qword ptr[param_1 + 0xb40]
		uintptr_t pGetEnemyThrowResistance = *(uintptr_t *)(param1);
		pGetEnemyThrowResistance = *(uintptr_t *)(pGetEnemyThrowResistance + 0xb40);
		const GetEnemyThrowResistanceType orig = (GetEnemyThrowResistanceType)pGetEnemyThrowResistance;
		if (isDEBUG) {
			if (orig != enemyThrowResFunc) {
				DebugBreak(); // verify we're calling the correct function
			}
		}

		const uint8_t origThrowRes = orig(param1);
		uint8_t easyThrowRes = origThrowRes / CONFIG.ThrowEnemy;
		if (easyThrowRes == 0) {
			// 0 and 1 will do the same, since the player's keypress counter is incremented to 1 before this function is called the first time.
			// Only reason I'm forcing easyThrowRes to 1 here, is to be on the safe side if this is ever called by another function that I'm not aware of.
			// (In case that hypothetical function expects a non-zero value as return)
			easyThrowRes++;
		}

		if (isDEBUG) {
			utils::Log(fmt::format("GetEnemyThrowResistance - TzNow: {:s} - {:d} - origThrowRes: {:d} - EnemyThrowResDiv: {:d} - easyThrowRes: {:d}", 
				utils::TzString_ms(), dbg_Counter1++, origThrowRes, CONFIG.ThrowEnemy, easyThrowRes), 1);
		}

		return easyThrowRes;
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

	/*
	* Patch instances where the game expects the player to spam press a key to complete an action.
	* The goal is to make these instances easier by lowering the amount of required keypresses.
	* 
	* SHA-1 checksums for compatible binaries:
	* 20d9614f41dc675848be46920974795481bdbd3b Yakuza3.exe (Steam)
	* 2a55a4b13674d4e62cda2ff86bc365d41b645a92 Yakuza3.exe (Steam without SteamStub)
	* 6c688b51650fa2e9be39e1d934e872602ee54799 Yakuza3.exe (GOG)
	*/
	{
		using namespace EasySpam;

		if (isDEBUG) {
			// Open debug logfile streams (not necessary but will save some time on the first real log message)
			utils::Log("", 1);
			utils::Log("", 2);
			utils::Log("", 3);
			utils::Log("", 4);

			// GetEnemyThrowResistance - to verify we're calling the correct function
			auto enemyThrowCheck = pattern("0f b6 81 ba 1c 00 00 c3");
			if (enemyThrowCheck.count_hint(1).size() == 1) {
				const auto match = enemyThrowCheck.get_one();
				enemyThrowResFunc = (GetEnemyThrowResistanceType)match.get<void>();
			}

			// AddHeat - to verify we're calling the correct function
			auto addHeatPattern = pattern("48 89 5c 24 08 57 48 83 ec 30 48 8b 01 8b fa 48 8b d9 ff 90 20 03 00 00");
			if (addHeatPattern.count_hint(1).size() == 1) {
				const auto match = addHeatPattern.get_one();
				addHeatFunc = (AddHeatType)match.get<void>();
			}
		}

		/*
		* ff 92 40 0b 00 00 3a d8
		* 
		* Call to function that returns the throw "resistance" of the enemy that the player is trying to throw.
		* Essentially the number of keypresses required to overpower and successfully throw the enemy.
		* Once the player's keypress counter is >= the enemy's throw resistance, the enemy will be thrown.
		* Won't be called for standard thugs since those can be thrown immediately.
		*/
		auto enemyThrowResistance = pattern("ff 92 40 0b 00 00 3a d8");
		if (enemyThrowResistance.count_hint(1).size() == 1) {
			utils::Log("Found pattern: GetEnemyThrowResistance");
			const auto match = enemyThrowResistance.get_one();
			Trampoline *trampoline = Trampoline::MakeTrampoline(match.get<void>());
			Nop(match.get<void>(), 6);
			InjectHook(match.get<void>(), trampoline->Jump(PatchedGetEnemyThrowResistance), PATCH_CALL);
		}

		/*
		* 84 c0 74 08 fe c0 88 81 ba 1c 00 00
		* 
		* After a successful throw by the player, the enemy's throw resistance will increase by 1.
		* So each successful throw will make the next one after a little more difficult.
		*/
		auto incThrowResistance = pattern("84 c0 74 08 fe c0 88 81 ba 1c 00 00");
		if (incThrowResistance.count_hint(1).size() == 1) {
			utils::Log("Found pattern: IncreaseThrowResistance");
			const auto match = incThrowResistance.get_one();
			const void *incAddr = match.get<void>(4);
			const void *retAddr = (void *)((uintptr_t)incAddr + 8);
			Trampoline *trampoline = Trampoline::MakeTrampoline(incAddr);

			// Nop these 2 instructions to make room for our JMP:
			// 0xfe, 0xc0 (INC AL)
			// 0x88, 0x81, 0xba, 0x1c, 0x00, 0x00 (MOV byte ptr[RCX + 0x1cba],AL)
			Nop(incAddr, 8);

			/*
			* Inject call to our function in order to modify how/when the enemy's throw resistance increases.
			* Now - it is obviously entirely unnecessary to inject a function call here,
			* since we can simply change the contents of RAX in assembly before writing them to memory.
			* But I like being able to write a log message whenever the game hits that code and I also wanna be able
			* to easily modify the return value depending on the settings loaded from the JSON configuration file.
			* 
			* Maybe I'm being dumb but I see no better way to inject a call to our function
			* while guaranteeing that the contents of all registers will stay the same. Basically:
			* - JMP to our payload
			* - back up volatile registers RCX, RDX, R8, R9, R10, R11 (RAX is volatile too, but no need to back it up here)
			* - back up RSP before alignment
			* - align RSP to 16-byte boundary (not necessary here but I like being safe)
			* - reserve 32 bytes of shadow space (otherwise our register backups on the stack could be overwritten by the callee)
			* - load address of our function into RAX
			* - call our function
			* - restore RSP and RCX, RDX, R8, R9, R10, R11
			* - write return value of our function to memory (i.e. overwriting the old throw resistance value)
			* - JMP back to the next instruction after our injected JMP
			* 
			* Had to do some reading for this one, since I didn't know about the importance of the shadow space
			* and that kept messing up the register backup stored on top of the stack.
			* https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention?view=msvc-170
			* https://learn.microsoft.com/en-us/cpp/build/prolog-and-epilog?view=msvc-170
			* https://g3tsyst3m.github.io/shellcoding/assembly/debugging/x64-Assembly-&-Shellcoding-101/
			* https://vimalshekar.github.io/reverse-engg/Assembly-Basics-Part11
			* https://sonictk.github.io/asm_tutorial/
			* https://stackoverflow.com/a/30191127
			* https://stackoverflow.com/a/30194393
			* https://medium.com/@sruthk/cracking-assembly-function-prolog-and-epilog-in-x64-ea1bdc50514c
			* https://www.reddit.com/r/asm/comments/1bff694/x64_calling_convention_and_shadow_space/
			* https://www.reddit.com/r/asm/comments/12o59gk/aligning_stack/
			* https://stackoverflow.com/a/64729675
			*/
			const uint8_t payload[] = {
				0x51, // push rcx (RCX here contains the address of the enemy actor object)
				0x52, // push rdx
				0x48, 0x89, 0xc1, // mov rcx, rax (copy current throw resistance to RCX/param1 for our function)
				0x41, 0x50, // push r8
				0x41, 0x51, // push r9
				0x41, 0x52, // push r10
				0x41, 0x53, // push r11
				0x48, 0x89, 0xe0, // mov rax, rsp (back up rsp before alignment)
				0x48, 0x83, 0xe4, 0xf0, // and rsp, -16 (align rsp to 16-byte boundary if necessary)
				0x50, // push rax (push old rsp contents onto the stack, which will unalign rsp again)
				0x48, 0x83, 0xec, 0x28, // sub rsp, 0x28 (0x20 shadow space for callee + 0x8 for stack alignment)
				0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // movabs rax, PatchedIncThrowResistance
				0xff, 0xd0, // call rax
				0x48, 0x83, 0xc4, 0x28, // add rsp, 0x28 ("unreserve" shadow space + manual alignment)
				0x5c, // pop rsp (restore rsp from before 16-byte boundary alignment)
				0x41, 0x5b, // pop r11
				0x41, 0x5a, // pop r10
				0x41, 0x59, // pop r9
				0x41, 0x58, // pop r8
				0x5a, // pop rdx
				0x59, // pop rcx
				0x88, 0x81, 0xba, 0x1c, 0x00, 0x00, // MOV byte ptr[RCX + 0x1cba], AL (write return value to memory)
				0xe9, 0x00, 0x00, 0x00, 0x00 // JMP retAddr
			};

			std::byte *space = trampoline->RawSpace(sizeof(payload));
			memcpy(space, payload, sizeof(payload));

			const auto funcAddr = utils::GetFuncAddr(PatchedIncThrowResistance);
			// 1 + 1 + 3 + 2 + 2 + 2 + 2 + 3 + 4 + 1 + 4 + 2 = 27
			memcpy(space + 27, &funcAddr, sizeof(funcAddr));

			WriteOffsetValue(space + sizeof(payload) - 4, retAddr);
			InjectHook(incAddr, space, PATCH_JUMP);
		}

		/*
		* 44 8d 47 06 eb 37 fe 8b 5e 1b 00 00
		* 
		* When the player is trying to escape an enemy's grab, each button press will decrease a counter by 1.
		* After this counter reaches 0, the player will escape the enemy's grab/hold with the next button press.
		*/
		auto decHoldPower = pattern("44 8d 47 06 eb 37 fe 8b 5e 1b 00 00");
		if (decHoldPower.count_hint(1).size() == 1) {
			utils::Log("Found pattern: DecreaseHoldPower");
			const auto match = decHoldPower.get_one();
			const void *decAddr = match.get<void>(6);
			const void *retAddr = (void *)((uintptr_t)decAddr + 6);
			const void *retAddrIfZero = (void *)((uintptr_t)decAddr - 6);
			Trampoline *trampoline = Trampoline::MakeTrampoline(decAddr);

			// Nop this instruction to make room for our JMP:
			// 0xfe, 0x8b, 0x5e, 0x1b, 0x00, 0x00 (DEC byte ptr [RBX + 0x1b5e])
			// RBX = address of player actor object
			Nop(decAddr, 6);

			/*
			* Inject call to our function in order to modify how much each button press will decrease the grab counter.
			* I've chosen to slightly modify the game's default behavior here. By default the game requires another key
			* press after the counter reaches 0, since the decrease happens after the check for 0. My modification
			* will let the player escape immediately once the counter hits 0.
			* 
			* Again - same reason as above for why I chose to inject a call here instead of modifying the value in assembly.
			* And again - the goal here is to have all registers as well as the stack remain unchanged once we return to the
			* game's own code. (Except for RDI, which we explicitly want to change if our function returned 0)
			*/
			const uint8_t payload[] = {
				0x50, // push rax
				0x51, // push rcx
				0x0f, 0xb6, 0x8b, 0x5e, 0x1b, 0x00, 0x00, // movzx ecx, BYTE PTR[rbx + 0x1b5e] (copy current hold power to RCX/param1)
				0x52, // push rdx
				0x41, 0x50, // push r8
				0x41, 0x51, // push r9
				0x41, 0x52, // push r10
				0x41, 0x53, // push r11
				0x48, 0x89, 0xe0, // mov rax, rsp (back up rsp before alignment)
				0x48, 0x83, 0xe4, 0xf0, // and rsp, -16 (align rsp to 16-byte boundary if necessary)
				0x50, // push rax (push old rsp contents onto the stack, which will unalign rsp again)
				0x48, 0x83, 0xec, 0x28, // sub rsp, 0x28 (0x20 shadow space for callee + 0x8 for stack alignment)
				0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // movabs rax, PatchedDecHoldPower
				0xff, 0xd0, // call rax
				0x48, 0x83, 0xc4, 0x28, // add rsp, 0x28 ("unreserve" shadow space + manual alignment)
				0x5c, // pop rsp (restore rsp from before 16-byte boundary alignment)
				0x88, 0x83, 0x5e, 0x1b, 0x00, 0x00, // mov BYTE PTR[rbx + 0x1b5e], al (write return value/new hold power to memory)
				0x3c, 0x00, // cmp al, 0x0 (check if return value was 0)
				0x41, 0x5b, // pop r11
				0x41, 0x5a, // pop r10
				0x41, 0x59, // pop r9
				0x41, 0x58, // pop r8
				0x5a, // pop rdx
				0x59, // pop rcx
				0x58, // pop rax
				0x74, 0x05, // jz/je +0x5 (skip next instruction if return value was 0)
				0xe9, 0x00, 0x00, 0x00, 0x00, // jmp retAddr
				0x48, 0x31, 0xff, // xor rdi, rdi (set RDI to 0 if return value was 0)
				0xe9, 0x00, 0x00, 0x00, 0x00 // jmp retAddrIfZero (jump to immediate release if return value was 0)
			};

			std::byte *space = trampoline->RawSpace(sizeof(payload));
			memcpy(space, payload, sizeof(payload));

			const auto funcAddr = utils::GetFuncAddr(PatchedDecHoldPower);
			// 1 + 1 + 7 + 1 + 2 + 2 + 2 + 2 + 3 + 4 + 1 + 4 + 2 = 32
			memcpy(space + 32, &funcAddr, sizeof(funcAddr));

			WriteOffsetValue(space + sizeof(payload) - 5 - 3 - 4, retAddr);
			WriteOffsetValue(space + sizeof(payload) - 4, retAddrIfZero);
			InjectHook(decAddr, space, PATCH_JUMP);
		}

		/*
		* ba 2c 01 00 00 ff 90 18 03 00 00 - Only used in chapter 1? Default charge amount is 300
		* ba 82 00 00 00 41 ff 90 18 03 00 00 - Used after chapter 1? Default charge amount is 130
		* 
		* During the charge "section" of "Feel the Heat" each press of the charge button will call the
		* AddHeat() function in order to add a fixed amount of Heat to the current Heat value.
		*/
		auto chargeFeelTheHeat_1 = pattern("ba 2c 01 00 00 ff 90 18 03 00 00");
		auto chargeFeelTheHeat_2 = pattern("ba 82 00 00 00 41 ff 90 18 03 00 00");
		if (chargeFeelTheHeat_1.count_hint(1).size() == 1) {
			utils::Log("Found pattern: ChargeFeelTheHeat1");
			const auto match = chargeFeelTheHeat_1.get_one();
			const void *callAddr = match.get<void>(5);
			Trampoline *trampoline = Trampoline::MakeTrampoline(callAddr);
			Nop(callAddr, 6);
			InjectHook(callAddr, trampoline->Jump(PatchedChargeFeelTheHeat), PATCH_CALL);
		}
		if (chargeFeelTheHeat_2.count_hint(1).size() == 1) {
			utils::Log("Found pattern: ChargeFeelTheHeat2");
			const auto match = chargeFeelTheHeat_2.get_one();
			const void *callAddr = match.get<void>(5);
			Trampoline *trampoline = Trampoline::MakeTrampoline(callAddr);
			Nop(callAddr, 7);
			InjectHook(callAddr, trampoline->Jump(PatchedChargeFeelTheHeat), PATCH_CALL);
		}
	}

	// log current time to file to get some feedback once hook is done
	utils::Log("Hook done!");
	utils::Log(fmt::format("Config path: \"{:s}\"", CONFIG.path));
	utils::Log(fmt::format("Local: {:s}", utils::TzString()));
	utils::Log(fmt::format("UTC:   {:s}", utils::UTCString()), true);
}
