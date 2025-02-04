#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#define WINVER 0x0601
#define _WIN32_WINNT 0x0601

#include <windows.h>
#include <ShlObj.h>

#include "Utils/MemoryMgr.h"
#include "Utils/Trampoline.h"
#include "Utils/Patterns.h"

#include <chrono>
#include <format>
#include <fstream>


namespace HeatFix {
	using namespace std;
#if _DEBUG
	using namespace std::chrono;
	static const auto tz = current_zone();
	static std::ofstream ofs1, ofs2;
#endif // _DEBUG
	static void(*origHeatFunc)(uintptr_t param1, uintptr_t param2, uintptr_t param3, uintptr_t param4);
	static int counter = 0;

	static void PatchedHeatFunc(uintptr_t param1, uintptr_t param2, uintptr_t param3, uintptr_t param4) {
#if _DEBUG
		if (!ofs1.is_open()) {
			ofs1 = ofstream("Heatfix_Debug1.txt", ios::out | ios::trunc | ios::binary);
		}
		if (!ofs2.is_open()) {
			ofs2 = ofstream("Heatfix_Debug2.txt", ios::out | ios::trunc | ios::binary);
		}
		const auto utcNow = system_clock::now();
		const auto tzNow = tz->to_local(utcNow);
		const auto str_TzNow = format("{:%Y/%m/%d %H:%M:%S}", tzNow);
		ofs1 << "PatchedHeatFunc: " << str_TzNow << endl;
#endif // _DEBUG
		if (counter % 2 == 0) {
			counter = 0;
#if _DEBUG
			ofs2 << "OrigHeatFunc: " << str_TzNow << endl;
#endif // _DEBUG
			origHeatFunc(param1, param2, param3, param4);
		}
		counter++;
	}
}


void OnInitializeHook()
{
	std::unique_ptr<ScopedUnprotect::Unprotect> Protect = ScopedUnprotect::UnprotectSectionOrFullModule(GetModuleHandle(nullptr), ".text");

	using namespace std;
	using namespace Memory;
	using namespace hook;

	{
		using namespace HeatFix;

		// e8 ?? ?? ?? ?? 48 8b 83 d0 13 00 00
		auto heatFunc = pattern("e8 ? ? ? ? 48 8b 83 d0 13 00 00");
		if (heatFunc.count_hint(1).size() == 1) {
			auto match = heatFunc.get_one();
			//ReadCall(match.get<void>(), origHeatFunc);
			//InjectHook(match.get<void>(), PatchedHeatFunc);

			Trampoline *trampoline = Trampoline::MakeTrampoline(match.get<void>());
			ReadCall(match.get<void>(), origHeatFunc);
			//newHeatFunc = PatchedHeatFunc;
			//ptrdiff_t offset = llabs((uintptr_t)match.get<void>() - (uintptr_t)PatchedHeatFunc);
			InjectHook(match.get<void>(), trampoline->Jump(PatchedHeatFunc), PATCH_CALL);

			//Trampoline *trampoline = Trampoline::MakeTrampoline(match.get<void>());
			//void **funcPtr = trampoline->Pointer<void *>();
			//*funcPtr = &PatchedHeatFunc;
			//WriteOffsetValue(match.get<void>(1), nullptr);
		}
	}

	// log current time to file to get some feedback once hook is done
	{
		using namespace std::chrono;
		const auto utcNow = system_clock::now();
		const auto str_UtcNow = format("{:%Y/%m/%d %H:%M:%S}", floor<seconds>(utcNow));
		// to local time, ref: https://akrzemi1.wordpress.com/2022/04/24/local-time/
		const auto tz = current_zone();
		const auto tzNow = tz->to_local(utcNow);
		const auto str_TzNow = format("{:%Y/%m/%d %H:%M:%S}", floor<seconds>(tzNow));
		auto ofs = std::ofstream("Heatfix.txt", ios::binary | ios::trunc | ios::out);
		ofs << "Hook done!" << endl;
		ofs << "Local: " << tzNow << endl;
		ofs << "UTC:   " << utcNow << endl;
		ofs.close();
	}
}
