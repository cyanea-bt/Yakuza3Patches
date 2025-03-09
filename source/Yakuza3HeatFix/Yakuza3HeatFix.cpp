#include "utils.h"
#include "Yakuza3.h"
#include "Yakuza3HeatFix.h"


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
				utils::Log("", 8, "AddHeatComboFinisher");
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
			* c5 32 58 0d ?? ?? ?? ?? e9 ?? ?? ?? ?? 48 8b - Heat boost by "Rescue Card"
			* c5 32 58 0d ?? ?? ?? ?? e9 ?? ?? ?? ?? f7 83 - Heat boost by "Phoenix Spirit"
			* 
			* Both of these automatically increase Heat while the player's health is at/below 19%.
			* Both have the same issue - they add a fixed amount of Heat every frame. So in Y3R
			* (which ALWAYS runs Heat calculations 60 times per second) they increase Heat at twice
			* their intended rate. Confirmed by comparison with PS3 version.
			*/
			auto addRescueCard = pattern("c5 32 58 0d ? ? ? ? e9 ? ? ? ? 48 8b");
			auto addPhoenixSpirit = pattern("c5 32 58 0d ? ? ? ? e9 ? ? ? ? f7 83");
			if (CONFIG.FixHeatGain && addRescueCard.count_hint(1).size() == 1) {
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
			if (CONFIG.FixHeatGain && addPhoenixSpirit.count_hint(1).size() == 1) {
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
			if (CONFIG.FixHeatGain && addLotusFlare.count_hint(1).size() == 1) {
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
			if (CONFIG.FixHeatGain && addGoldenDragonSpirit.count_hint(1).size() == 1) {
				utils::Log("Found pattern: GoldenDragonSpirit");
				const auto match = addGoldenDragonSpirit.get_one();
				void *pImmediate = match.get<void>(1); // immediate value for mov edx, immediate
				uint32_t origValue;
				memcpy(&origValue, pImmediate, sizeof(origValue));
				const uint32_t newValue = origValue / integerDiv;
				memcpy(pImmediate, &newValue, sizeof(newValue));
			}
			if (CONFIG.FixHeatGain && addLeechGloves.count_hint(1).size() == 1) {
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
			auto addLongPressComboFinisher = pattern("c5 fa 2d d1 48 8b cf ff 90 18 03 00 00");
			if (CONFIG.FixHeatGain && addLongPressComboFinisher.count_hint(1).size() == 1) {
				utils::Log("Found pattern: LongPressComboFinisher");
				const auto match = addLongPressComboFinisher.get_one();
				const void *callAddr = match.get<void>(7);
				Nop(callAddr, 6);
				Trampoline *trampoline = Trampoline::MakeTrampoline(callAddr);
				InjectHook(callAddr, trampoline->Jump(AddHeatComboFinisher), PATCH_CALL);
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
				utils::Log("Found pattern: UpdateHeat");
				const auto match = finalHeatCalc.get_one();
				const void *callAddr = match.get<void>(4);
				const void *retAddr = match.get<void>(10);
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

				const auto pGetNewHeatValue = utils::GetFuncAddr(GetNewHeatValue);
				// 3 + 3 + 3 + 2 = 11
				memcpy(space + 11, &pGetNewHeatValue, sizeof(pGetNewHeatValue));

				const auto pGetMaxHeatValue = utils::GetFuncAddr(GetMaxHeatValue);
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
	Yakuza3::Finalize("Hook done!");
}
