﻿#define WIN32_LEAN_AND_MEAN
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
static const auto tz = std::chrono::current_zone();
static constexpr std::string_view tzFmt("{:%Y/%m/%d %H:%M:%S}");

static std::string getUTCString() {
	using namespace std::chrono;
	const auto utcNow = system_clock::now();
	return std::format(tzFmt, floor<seconds>(utcNow));
}

static std::string getUTCString_ms() {
	using namespace std::chrono;
	const auto utcNow = system_clock::now();
	return std::format(tzFmt, utcNow);
}

static std::string getTzString() {
	using namespace std::chrono;
	const auto utcNow = system_clock::now();
	const auto tzNow = tz->to_local(utcNow);
	return std::format(tzFmt, floor<seconds>(tzNow));
}

static std::string getTzString_ms() {
	using namespace std::chrono;
	const auto utcNow = system_clock::now();
	const auto tzNow = tz->to_local(utcNow);
	return std::format(tzFmt, tzNow);
}


namespace EasySpam {
	using namespace std;
	
	static ofstream ofs1, ofs2, ofs3, ofs4;
	static uint64_t dbg_Counter1 = 0, dbg_Counter2 = 0, dbg_Counter3 = 0, dbg_Counter4 = 0;
	static string dbg_msg;

	typedef uint8_t(*GetEnemyThrowResistanceType)(uintptr_t);
	GetEnemyThrowResistanceType enemyThrowResFunc = nullptr;

	static uint8_t PatchedDecHoldPower(uint8_t oldHoldPower) {
		uint8_t newHoldPower = 0;
		if (oldHoldPower > 2) {
			newHoldPower = oldHoldPower - 2;
		}

		if (s_Debug) {
			if (oldHoldPower > 40) {
				DebugBreak(); // Anything higher than ~20 is probably wrong
			}
			if (ofs2.is_open()) {
				ofs2 << format("PatchedDecHoldPower - TzNow: {:s} - {:d} - oldHoldPower: {:d} - newHoldPower: {:d}", getTzString_ms(), dbg_Counter2++, oldHoldPower, newHoldPower) << endl;
			}
		}

		return newHoldPower;
	}

	static uint8_t PatchedIncThrowResistance(uint8_t oldThrowRes) {
		uint8_t newThrowRes;
		if (oldThrowRes == UINT8_MAX) {
			newThrowRes = oldThrowRes;
		}
		else {
			newThrowRes = oldThrowRes + 1;
		}

		if (s_Debug) {
			if (oldThrowRes > 80) {
				DebugBreak(); // Anything higher than ~40 is probably wrong
			}
			if (ofs1.is_open()) {
				ofs1 << format("PatchedIncThrowResistance - TzNow: {:s} - {:d} - oldThrowRes: {:d} - newThrowRes: {:d}", getTzString_ms(), dbg_Counter1++, oldThrowRes, newThrowRes) << endl;
			}
		}

		return newThrowRes;
	}

	// param1 here is a pointer to the actor object of the enemy that the player is trying to throw
	static uint8_t PatchedGetEnemyThrowResistance(uintptr_t param1) {
		// CALL qword ptr[param_1 + 0xb40]
		uintptr_t pGetEnemyThrowResistance = *(uintptr_t *)(param1);
		pGetEnemyThrowResistance = *(uintptr_t *)(pGetEnemyThrowResistance + 0xb40);
		const GetEnemyThrowResistanceType orig = (GetEnemyThrowResistanceType)pGetEnemyThrowResistance;

		const uint8_t origThrowRes = orig(param1);
		uint8_t easyThrowRes = origThrowRes / 2;
		if (easyThrowRes == 0) {
			// 0 and 1 will do the same, since the player's keypress counter is incremented to 1 before this function is called the first time.
			// Only reason I'm forcing easyThrowRes to 1 here, is to be on the safe side if this is ever called by another function that I'm not aware of.
			// (In case that hypothetical function expects a non-zero value as return)
			easyThrowRes++;
		}

		if (s_Debug) {
			if (orig != enemyThrowResFunc) {
				DebugBreak();
			}
			if (ofs1.is_open()) {
				ofs1 << format("GetEnemyThrowResistance - TzNow: {:s} - {:d} - origThrowRes: {:d} - easyThrowRes: {:d}", getTzString_ms(), dbg_Counter1++, origThrowRes, easyThrowRes) << endl;
			}
		}

		return easyThrowRes;
	}
}

namespace HeatFix {
	using namespace std;
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
				dbg_msg = format("- TzNow: {:s} - IsPlayerInCombat: {:d} - IsCombatInactive: {:d} - isCombatPausedByTutorial: {:d} - IsActorDead: {:d} - isCombatInTransition: {:d} - isCombatFinished: {:d}", 
					getTzString_ms(), IsPlayerInCombat(), IsCombatInactive(), isCombatPausedByTutorial, IsActorDead(param1), isCombatInTransition, isCombatFinished);
				ofs1 << "PatchedHeatFunc: " << dbg_Counter1++ << dbg_msg << endl;
				if (counter % 2 == 0) {
					ofs2 << "OrigHeatFunc: " << dbg_Counter2++ << dbg_msg << endl;
				}
			}
		}

		if (!IsPlayerInCombat()) {
			counter = 0;
			return; // HeatUpdate() will return immediately in this case, so we just skip calling it
		}
		else if (counter == 1 && (IsCombatInactive() || isCombatPausedByTutorial || IsActorDead(param1) || isCombatInTransition || isCombatFinished)) {
			if (s_Debug) {
				if (ofs2.is_open()) {
					ofs2 << "Fast HeatUpdate: " << dbg_Counter2++ << dbg_msg << endl;
				}
			}
			// HeatUpdate() won't change the Heat value in these cases, but will still execute some code.
			// Since I don't know how important that code is, we'll call HeatUpdate() immediately instead of waiting for the next frame/update.
			origHeatFunc(param1, param2, param3, param4);
			counter++;
		}
		else {
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
}


// Can't seem to get the correct function address at runtime without this
// ref: Utils/Trampoline.h
template<typename Func>
LPVOID GetFuncAddr(Func func)
{
	LPVOID addr;
	memcpy(&addr, std::addressof(func), sizeof(addr));
	return addr;
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

	// Game/window name taken from https://github.com/CookiePLMonster/SilentPatchYRC/blob/ae9201926134445f247be42c6f812dc945ad052b/source/SilentPatchYRC.cpp#L396
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
			if (game != Game::Yakuza3) {
				ofs << "Game is NOT Yakuza 3, HeatFix was disabled!" << endl;
			}
			else {
				ofs << "HeatFix was disabled!" << endl;
			}
			if (s_Debug) {
				ofs << endl << format("Config path: \"{:s}\"", config.path) << endl;
			}
			ofs << "Local: " << getTzString() << endl;
			ofs << "UTC:   " << getUTCString() << endl;
		}
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

		if (s_Debug) {
			ofs1 = ofstream(format("{:s}{:s}", rsc_Name, "_Debug1.txt"), ios::out | ios::trunc | ios::binary);
			ofs2 = ofstream(format("{:s}{:s}", rsc_Name, "_Debug2.txt"), ios::out | ios::trunc | ios::binary);

			// GetEnemyThrowResistance - to verify we're calling the correct function
			auto enemyThrowCheck = pattern("0f b6 81 ba 1c 00 00 c3");
			if (enemyThrowCheck.count_hint(1).size() == 1) {
				auto match = enemyThrowCheck.get_one();
				enemyThrowResFunc = (GetEnemyThrowResistanceType)match.get<void>();
			}
		}

		/*
		* ff 92 40 0b 00 00 3a d8
		* 
		* Call to function that returns the throw "resistance" of the enemy that the player is trying to throw.
		* Essentially the number of keypresses required to overpower and successfully throw the enemy.
		* Won't be called for standard thugs since those can be thrown immediately.
		*/
		auto enemyThrowResistance = pattern("ff 92 40 0b 00 00 3a d8");
		if (enemyThrowResistance.count_hint(1).size() == 1) {
			if (ofs.is_open()) {
				ofs << "Found pattern: GetEnemyThrowResistance" << endl;
			}
			auto match = enemyThrowResistance.get_one();
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
			if (ofs.is_open()) {
				ofs << "Found pattern: IncreaseThrowResistance" << endl;
			}
			auto match = incThrowResistance.get_one();
			void *incAddr = match.get<void>(4);
			void *retAddr = (void *)((uintptr_t)incAddr + 8);
			Trampoline *trampoline = Trampoline::MakeTrampoline(incAddr);

			// Nop these 2 instructions to make room for our JMP:
			// 0xfe, 0xc0 (INC AL)
			// 0x88, 0x81, 0xba, 0x1c, 0x00, 0x00 (MOV byte ptr[RCX + 0x1cba],AL)
			Nop(incAddr, 8);

			/*
			* Inject call to our function in order to modify how/when the enemy's throw resistance increases.
			* Now - it is obviously entirely unnecessary to inject a function call here,
			* since we can simply change the contents of RAX in assembly before writing them to memory.
			* But I like being able to write a log message whenever the game hits that code.
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

			LPVOID funcAddr = GetFuncAddr(PatchedIncThrowResistance);
			memcpy(space + 2 + 3 + 8 + 3 + 4 + 1 + 4 + 2, &funcAddr, sizeof(funcAddr));

			WriteOffsetValue(space + sizeof(payload) - 4, retAddr);
			InjectHook(incAddr, space, PATCH_JUMP);
		}

		/*
		* fe 8b 5e 1b 00 00 eb 4d
		* 
		* When the player is trying to escape an enemy's grab, each button press will decrease a counter by 1.
		* Once this counter reaches 0 the player will escape the enemy's grab/hold.
		*/
		auto decHoldPower = pattern("fe 8b 5e 1b 00 00 eb 4d");
		if (decHoldPower.count_hint(1).size() == 1) {
			if (ofs.is_open()) {
				ofs << "Found pattern: DecreaseHoldPower" << endl;
			}
			auto match = decHoldPower.get_one();
			void *decAddr = match.get<void>();
			void *retAddr = (void *)((uintptr_t)decAddr + 6);
			Trampoline *trampoline = Trampoline::MakeTrampoline(decAddr);

			// Nop this instruction to make room for our JMP:
			// 0xfe, 0x8b, 0x5e, 0x1b, 0x00, 0x00 (DEC byte ptr [RBX + 0x1b5e])
			// RBX = address of player actor object
			Nop(decAddr, 6);

			// Inject call to our function in order to modify how much each button press will decrease the grab counter.
			// Since the decrease happens after the escape check, at least 2 button presses will always be required to escape.
			const uint8_t payload[] = {
				0x50, // push rax
				0x51, // push rcx
				0x0f, 0xb6, 0x8b, 0x5e, 0x1b, 0x00, 0x00, // movzx ecx, BYTE PTR[rbx + 0x1b5e] (copy hold power to RCX/param1)
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
				0x88, 0x83, 0x5e, 0x1b, 0x00, 0x00, // mov BYTE PTR[rbx + 0x1b5e], al (write return value to memory)
				0x41, 0x5b, // pop r11
				0x41, 0x5a, // pop r10
				0x41, 0x59, // pop r9
				0x41, 0x58, // pop r8
				0x5a, // pop rdx
				0x59, // pop rcx
				0x58, // pop rax
				0xe9, 0x00, 0x00, 0x00, 0x00 // jmp retAddr
			};

			std::byte *space = trampoline->RawSpace(sizeof(payload));
			memcpy(space, payload, sizeof(payload));

			LPVOID funcAddr = GetFuncAddr(PatchedDecHoldPower);
			memcpy(space + 2 + 7 + 9 + 3 + 4 + 1 + 4 + 2, &funcAddr, sizeof(funcAddr));

			WriteOffsetValue(space + sizeof(payload) - 4, retAddr);
			InjectHook(decAddr, space, PATCH_JUMP);
		}
	}

	// log current time to file to get some feedback once hook is done
	if (ofs.is_open()) {
		ofs << "Hook done!" << endl;
		if (s_Debug) {
			ofs << endl << format("Config path: \"{:s}\"", config.path) << endl;
		}
		ofs << "Local: " << getTzString() << endl;
		ofs << "UTC:   " << getUTCString() << endl;
	}
}
