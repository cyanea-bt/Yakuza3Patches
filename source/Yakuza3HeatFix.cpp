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
	static void(*origHeatFunc)(uintptr_t param1, uintptr_t param2, uintptr_t param3, uintptr_t param4);
	//static void(*newHeatFunc)(uintptr_t param1, uintptr_t param2, uintptr_t param3, uintptr_t param4);
	static int counter = 0;

	static std::ofstream ofs;

	static void PatchedHeatFunc(uintptr_t param1, uintptr_t param2, uintptr_t param3, uintptr_t param4) {
		if (!ofs.is_open()) {
			ofs = std::ofstream("Heatfix.txt", std::ios::out | std::ios::trunc | std::ios::binary);
		}

		ofs << "PatchedHeatFunc" << std::endl;
		if (counter % 2 == 0) {
			counter = 0;
			ofs << "OrigHeatFunc" << std::endl;
			origHeatFunc(param1, param2, param3, param4);
		}
		counter++;
	}
}


void OnInitializeHook()
{
	std::unique_ptr<ScopedUnprotect::Unprotect> Protect = ScopedUnprotect::UnprotectSectionOrFullModule( GetModuleHandle( nullptr ), ".text" );

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
		const auto t1 = system_clock::now();
		const auto s1 = std::format("{:%Y/%m/%d %H:%M:%S}", floor<seconds>(t1));
		// to local time, ref: https://akrzemi1.wordpress.com/2022/04/24/local-time/
		const auto tz = current_zone();
		const auto t2 = tz->to_local(t1);
		const auto s2 = std::format("{:%Y/%m/%d %H:%M:%S}", floor<seconds>(t2));
		auto ofs = std::ofstream("SilentPatchYRC.txt", std::ios::binary | std::ios::trunc | std::ios::out);
		ofs << "Hook done!" << std::endl;
		ofs << "Local: " << s2 << std::endl;
		ofs << "UTC:   " << s1 << std::endl;
		ofs.close();
	}
}
