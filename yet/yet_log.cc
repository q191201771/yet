#include "yet_log.h"

namespace yet {

std::shared_ptr<spdlog::logger> Log::core_;

std::shared_ptr<spdlog::logger> Log::instance() {
  // let caller ensure no double init issues.
  if (!core_) {
    core_ = spdlog::stdout_color_mt("yet");
    core_->set_level(spdlog::level::trace);
  }
  return core_;
}

void Log::dispose() {
  if (core_) {
    core_.reset();
  }
}

}
