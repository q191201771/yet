#include "yet_config.h"

namespace yet {

Config *Config::core_ = nullptr;

Config *Config::instance() {
  // let caller ensure no double new issues.
  if (!core_) {
    core_ = new Config();
  }
  return core_;
}

void Config::dispose() {
  if (core_) { delete core_; }
}

}
