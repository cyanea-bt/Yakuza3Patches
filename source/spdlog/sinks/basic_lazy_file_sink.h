//
// spdlog Copyright(c) 2015-2025 Gabi Melman & spdlog contributors, see https://github.com/gabime/spdlog
// This is a simple lazy variant of https://github.com/gabime/spdlog/blob/v1.x/include/spdlog/sinks/basic_file_sink.h
// Heavily inspired by https://gist.github.com/gexclaude/4f867c4b288c4f1c1426a2ce654ee3e6
//
#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/details/file_helper.h>
#include <spdlog/details/null_mutex.h>
#include <spdlog/details/synchronous_factory.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/common.h>
#include <spdlog/details/os.h>
#include <filesystem>
#include <mutex>
#include <string>


namespace spdlog {
	namespace sinks {
		//
		// Trivial lazy initialized file sink with single file as target
		//
		template<typename Mutex>
		class basic_lazy_file_sink final : public base_sink<Mutex> {
		public:
			explicit basic_lazy_file_sink(const filename_t &filename, bool truncate = false, const file_event_handlers &event_handlers = {})
				: filename_{ filename }, truncate_{ truncate }, file_helper_{ event_handlers } {}

			const filename_t &filename() const {
				return filename_;
			}

			void truncate() {
				if (initialized_) {
					std::lock_guard<Mutex> lock(this->mutex_);
					file_helper_.reopen(true);
				}
			}

		protected:
			void sink_it_(const details::log_msg &msg) override {
				if(!initialized_) {
					try {
						file_helper_.open(filename_, truncate_);
						initialized_ = true;
					}
					catch (const spdlog_ex &ex) {
						error(ex.what());
						// file open failed, try fallback log file inside temp directory once
						if (!fopen_failed_) {
							fopen_failed_ = true;
							const auto temp_path = std::filesystem::temp_directory_path() / filename_;
							filename_ = temp_path.string();
							file_helper_.open(filename_, truncate_);
							// trade off of a lazy initialized file logger - if last attempt fails exception is thrown and swallowed by spdlog
							initialized_ = true;
						}
					}
				}

				if(initialized_) {
					memory_buf_t formatted;
					this->formatter_->format(msg, formatted);
					file_helper_.write(formatted);
				}
			}

			void flush_() override {
				if (initialized_) {
					file_helper_.flush();
				}
			}

		private:
			filename_t filename_;
			bool truncate_ = false;
			details::file_helper file_helper_;
			bool fopen_failed_ = false;
			bool initialized_ = false;
		};

		using basic_lazy_file_sink_mt = basic_lazy_file_sink<std::mutex>;
		using basic_lazy_file_sink_st = basic_lazy_file_sink<details::null_mutex>;

	} // namespace sinks


	//
	// factory functions
	//
	template<typename Factory = spdlog::synchronous_factory>
	inline std::shared_ptr<logger> basic_lazy_logger_mt(
		const std::string &logger_name, 
		const filename_t &filename, 
		bool truncate = false, 
		const file_event_handlers &event_handlers = {}) 
	{
		return Factory::template create<sinks::basic_lazy_file_sink_mt>(logger_name, filename, truncate, event_handlers);
	}

	template<typename Factory = spdlog::synchronous_factory>
	inline std::shared_ptr<logger> basic_lazy_logger_st(
		const std::string &logger_name, 
		const filename_t &filename, 
		bool truncate = false, 
		const file_event_handlers &event_handlers = {}) 
	{
		return Factory::template create<sinks::basic_lazy_file_sink_st>(logger_name, filename, truncate, event_handlers);
	}

} // namespace spdlog
