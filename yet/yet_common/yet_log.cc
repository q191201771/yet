#include "yet_log.h"

namespace yet {

std::shared_ptr<spdlog::logger> Log::core_;

std::shared_ptr<spdlog::logger> Log::instance() {
  // let caller ensure no double init issues.
  if (!core_) {
    core_ = spdlog::stdout_color_st("yet");
    core_->set_level(spdlog::level::trace);
    core_->set_pattern("[%H:%M:%S.%f] [%^%l%$] %v");
    YET_LOG_DEBUG("debug log.");
    YET_LOG_INFO("info log.");
    YET_LOG_WARN("warn log.");
    YET_LOG_ERROR("error log.");
    YET_LOG_ASSERT(0, "assert log.");
  }
  return core_;
}

void Log::dispose() {
  if (core_) {
    core_.reset();
  }
}

}
