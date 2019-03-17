#include "yet_log.h"

namespace yet {

std::shared_ptr<spdlog::logger> Log::core_;

std::shared_ptr<spdlog::logger> Log::instance() {
  // let caller ensure no double init issues.
  if (!core_) {
    core_ = spdlog::stdout_color_st("yet");
    core_->set_level(spdlog::level::trace);
    core_->set_pattern("[%H:%M:%S.%f] [%^%l%$] %v");
    YET_LOG_DEBUG("I am a debug level log message.");
    YET_LOG_INFO("I am a info level log message.");
    YET_LOG_WARN("I am a warn level log message.");
    YET_LOG_ERROR("I am a error level log message.");
    YET_LOG_ASSERT(0, "I am a assert log message.");
  }
  return core_;
}

void Log::dispose() {
  if (core_) {
    core_.reset();
  }
}

}
