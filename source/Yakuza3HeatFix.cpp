#define WIN32_LEAN_AND_MEAN
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

#if _DEBUG
static constexpr bool s_Debug = true;
#else
static constexpr bool s_Debug = false;
#endif // _DEBUG


namespace HeatFix {
	using namespace std;
	using namespace std::chrono;
	static const auto tz = current_zone();
	static ofstream ofs1, ofs2;

	/*
	* Parameter types aren't correct but it shouldn't matter for our purposes.
	* param1 is probably a pointer to some kind of class/struct
	* No clue what the other 3 are, but all 4 parameters seem to be 8 bytes wide.
	*/
	static void(*origHeatFunc)(uintptr_t param1, uintptr_t param2, uintptr_t param3, uintptr_t param4);
	static int counter = 0;

	static void PatchedHeatFunc(uintptr_t param1, uintptr_t param2, uintptr_t param3, uintptr_t param4) {
		if (s_Debug) {
			if (!ofs1.is_open()) {
				ofs1 = ofstream("Yakuza3HeatFix_Debug1.txt", ios::out | ios::trunc | ios::binary);
			}
			if (!ofs2.is_open()) {
				ofs2 = ofstream("Yakuza3HeatFix_Debug2.txt", ios::out | ios::trunc | ios::binary);
			}
			const auto utcNow = system_clock::now();
			const auto tzNow = tz->to_local(utcNow);
			const string str_TzNow = format("{:%Y/%m/%d %H:%M:%S}", tzNow);
			ofs1 << "PatchedHeatFunc: " << str_TzNow << endl;
			if (counter % 2 == 0) {
				ofs2 << "OrigHeatFunc: " << str_TzNow << endl;
			}
		}
		/*
		* Y3 on PS3 runs at 30 fps during gameplay (60 fps in menus, though that hardly matters here).
		* Y3R always runs its internal logic 60 times per second (even when setting framelimit to 30 fps).
		* While "remastering" Y3, the HeatUpdate function (along with many other functions) wasn't rewritten
		* to take 60 updates per second into account. This leads to at least 2 issues in regards to the Heat bar:
		* - Heat value starts to drop too soon (should start after ~4 seconds but instead starts after ~2 seconds in the "Remaster")
		* - When Heat starts dropping, the drain rate is too fast when compared to the PS3 version.
		* 
		* Proper way to fix this would be to rewrite the HeatUpdate function (which is responsible for both of these issues)
		* to handle 60 updates per second. But that is definitely more work than I'm willing to put in at the moment.
		* Instead - we'll only run the HeatUpdate function every 2nd time, essentially halving the rate of Heat calculations.
		* This seems to work fine and the new drain rate looks pretty similar to the PS3 original.
		*/
		if (counter % 2 == 0) {
			counter = 0;
			origHeatFunc(param1, param2, param3, param4);
		}
		counter++;
	}
}


void OnInitializeHook()
{
	using namespace std;
	using namespace Memory;
	using namespace hook;

	unique_ptr<ScopedUnprotect::Unprotect> Protect = ScopedUnprotect::UnprotectSectionOrFullModule(GetModuleHandle(nullptr), ".text");
	ofstream ofs = ofstream("Yakuza3HeatFix.txt", ios::binary | ios::trunc | ios::out);
	{
		namespace fs = std::filesystem;
		fs::path manualDisable1("Yakuza3HeatFix.Disable");
		fs::path manualDisable2("Yakuza3HeatFix.disable");
		fs::path manualDisable3("Yakuza3HeatFix.Disable.txt");
		fs::path manualDisable4("Yakuza3HeatFix.disable.txt");
		if (fs::exists(manualDisable1) || fs::exists(manualDisable2) || fs::exists(manualDisable3) || fs::exists(manualDisable4)) {
			using namespace std::chrono;

			const auto utcNow = system_clock::now();
			const auto str_UtcNow = format("{:%Y/%m/%d %H:%M:%S}", floor<seconds>(utcNow));
			const auto tzNow = HeatFix::tz->to_local(utcNow);
			const auto str_TzNow = format("{:%Y/%m/%d %H:%M:%S}", floor<seconds>(tzNow));
			ofs << "HeatFix was disabled!" << endl;
			ofs << "Local: " << str_TzNow << endl;
			ofs << "UTC:   " << str_UtcNow << endl;
			return;
		}
	}

	{
		using namespace HeatFix;
		/*
		* e8 ?? ?? ?? ?? 48 8b 83 d0 13 00 00
		* Pattern works for Steam and GOG versions. Sha1 checksums for binaries:
		* 
		* 20d9614f41dc675848be46920974795481bdbd3b Yakuza3.exe (Steam)
		* 2a55a4b13674d4e62cda2ff86bc365d41b645a92 Yakuza3.exe (Steam without SteamStub)
		* 6c688b51650fa2e9be39e1d934e872602ee54799 Yakuza3.exe (GOG)
		*/
		auto updateHeat = pattern("e8 ? ? ? ? 48 8b 83 d0 13 00 00");
		if (updateHeat.count_hint(1).size() == 1) {
			ofs << "Found HeatUpdate pattern!" << endl;
			auto match = updateHeat.get_one();
			Trampoline *trampoline = Trampoline::MakeTrampoline(match.get<void>());
			ReadCall(match.get<void>(), origHeatFunc);
			InjectHook(match.get<void>(), trampoline->Jump(PatchedHeatFunc), PATCH_CALL);
		}
	}

	// log current time to file to get some feedback once hook is done
	{
		using namespace std::chrono;

		const auto utcNow = system_clock::now();
		const auto str_UtcNow = format("{:%Y/%m/%d %H:%M:%S}", floor<seconds>(utcNow));
		const auto tzNow = HeatFix::tz->to_local(utcNow);
		const auto str_TzNow = format("{:%Y/%m/%d %H:%M:%S}", floor<seconds>(tzNow));
		ofs << "Hook done!" << endl;
		ofs << "Local: " << str_TzNow << endl;
		ofs << "UTC:   " << str_UtcNow << endl;
	}
}
