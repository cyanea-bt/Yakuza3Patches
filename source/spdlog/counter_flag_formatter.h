#pragma once

#ifndef FMT_COMPILE_H_
#include <fmt/compile.h>
#endif

#ifndef SPDLOG_H
#include <spdlog/spdlog.h>
#endif

#include <spdlog/pattern_formatter.h>

namespace spdlog {
	// Custom flag with counter that gets incremented after each use
	// ref: https://github.com/gabime/spdlog/wiki/3.-Custom-formatting
	template <uint8_t minWidth>
	class counter_flag final : public custom_flag_formatter {
	public:
		void format(const details::log_msg &, const std::tm &, memory_buf_t &dest) override {
			constexpr auto formatStringBuf = getFormatString(minWidth);
			const std::string strCounter = fmt::format(formatStringBuf.data(), counter++);
			dest.append(strCounter.data(), strCounter.data() + strCounter.size());
		}

		std::unique_ptr<custom_flag_formatter> clone() const override {
			auto cloned = details::make_unique<counter_flag>();
			cloned->counter = counter;
			return cloned;
		}

	private:
		uint64_t counter = 0;

		// evaluate format string at compile time
		// ref: https://www.reddit.com/r/cpp/comments/kpejif/discussion_on_possibility_of_a_compiletime_printf/ghzdo4f/
		//      https://stackoverflow.com/a/68207254
		static consteval auto getFormatString(uint8_t n) {
			auto buf = std::array<char, 16>();
			auto result = fmt::format_to(buf.data(), FMT_COMPILE("{{:0{:d}d}}"), n);
			*result = '\0'; // make sure buf is zero-terminated (should already be the case)
			return buf;
		}
	};
} // namespace spdlog
