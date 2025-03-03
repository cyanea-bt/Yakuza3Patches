#include "utils.h"
#include "Yakuza3.h"


namespace Yakuza3 {
	using namespace std;
	using namespace hook;
	using namespace Memory;

	bool Init() {
		// Replace default spdlog logger
		try {
			const auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
			stdout_sink->set_level(spdlog::level::info);
			const auto msvc_sink = make_shared<spdlog::sinks::msvc_sink_mt>(); // MSVC debug sink
			msvc_sink->set_level(spdlog::level::trace);
			std::vector<spdlog::sink_ptr> sinks;
			sinks.push_back(stdout_sink);
			sinks.push_back(msvc_sink);
			const auto combined_logger = std::make_shared<spdlog::logger>(rsc_Name, begin(sinks), end(sinks));
			combined_logger->set_level(spdlog::level::trace);
			combined_logger->flush_on(spdlog::level::trace); // always flush immediately, no matter the log level
			spdlog::set_default_logger(combined_logger);
		}
		catch (const spdlog::spdlog_ex &ex)
		{
			(void)ex;
			if (isDEBUG) {
				DebugBreak(); // shouldn't happen
				utils::Log("");
			}
		}
		// Get config from file
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
		if ((game != Game::Yakuza3 && !CONFIG.IgnoreGameCheck) || !CONFIG.EnablePatch) {
			if (game != Game::Yakuza3) {
				utils::Log(fmt::format("Game is NOT {:s}, {:s} was disabled!", "Yakuza 3", rsc_Name));
			}
			else {
				utils::Log(fmt::format("{:s} was disabled!", rsc_Name));
			}
			utils::Log(fmt::format("\nConfig path: \"{:s}\"", CONFIG.path));
			utils::Log(fmt::format("Local: {:s}", utils::TzString()));
			utils::Log(fmt::format("UTC:   {:s}", utils::UTCString()));
			return false;
		}
		return true;
	}
}
