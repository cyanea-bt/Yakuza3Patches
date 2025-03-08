#include "utils.h"
#include "Yakuza3.h"


namespace Yakuza3 {
	using namespace std;
	using namespace hook;
	using namespace Memory;

	bool Init() {
		// Replace default spdlog logger
		try {
			const auto stdout_sink = make_shared<spdlog::sinks::stdout_color_sink_mt>();
			stdout_sink->set_level(spdlog::level::info);
			vector<spdlog::sink_ptr> sinks;
			sinks.push_back(stdout_sink);
			if (isDEBUG) {
				// MSVC debug sink
				const auto msvc_sink = make_shared<spdlog::sinks::msvc_sink_mt>();
				msvc_sink->set_level(spdlog::level::trace);
				sinks.push_back(msvc_sink);
				// logfile for debug warnings/errors
				const string filename = fmt::format("{:s}_{:s}.txt", rsc_Name, utils::TzFilename());
				const auto error_sink = make_shared<spdlog::sinks::basic_lazy_file_sink_mt>(filename);
				error_sink->set_level(spdlog::level::warn);
				error_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e][%l] %v");
				sinks.push_back(error_sink);
			}			
			const auto combined_logger = make_shared<spdlog::logger>(rsc_Name, begin(sinks), end(sinks));
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
				Done(fmt::format("Game is NOT {:s}, {:s} was disabled!", rsc_Game, rsc_Name));
			}
			else {
				Done(fmt::format("{:s} was disabled!", rsc_Name));
			}
			return false;
		}
		return true;
	}

	void Done(string_view finalMsg) {
		// log current time to file to get some feedback once hook is done
		utils::Log(finalMsg);
		utils::Log(fmt::format("\nConfig path: \"{:s}\"", CONFIG.path));
		utils::Log(fmt::format("Local: {:s}", utils::TzString()));
		utils::Log(fmt::format("UTC:   {:s}", utils::UTCString()), true);
	}
}
