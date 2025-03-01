#include "utils.h"
#include "Yakuza3.h"


config::Config CONFIG;

namespace Yakuza3 {
	using namespace std;
	using namespace hook;
	using namespace Memory;

	bool Init() {
		CONFIG = config::GetConfig();

		// Game detection taken from https://github.com/CookiePLMonster/SilentPatchYRC/blob/ae9201926134445f247be42c6f812dc945ad052b/source/SilentPatchYRC.cpp#L396
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
				game = windowName == string_view("Yakuza 4") ? Game::Yakuza4 : Game::Yakuza3;
			}
			else
			{
				// Not found? Most likely Yakuza 5
				// Not supported yet
				game = Game::Yakuza5;
			}
		}

		// Check if patch should be disabled
		if ((game != Game::Yakuza3 && !CONFIG.ForcePatch) || !CONFIG.EnablePatch) {
			if (game != Game::Yakuza3) {
				utils::Log(format("Game is NOT {:s}, {:s} was disabled!", "Yakuza 3", rsc_Name));
			}
			else {
				utils::Log(format("{:s} was disabled!", rsc_Name));
			}
			if (isDEBUG) {
				utils::Log(format("\nConfig path: \"{:s}\"", CONFIG.path));
			}
			utils::Log(format("Local: {:s}", utils::TzString()));
			utils::Log(format("UTC:   {:s}", utils::UTCString()));
			return false;
		}
		return true;
	}
}
