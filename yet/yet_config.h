/**
 * @file   yet_config.h
 * @author pengrl
 * @date   20190127
 *
 */

#pragma once

#include <string>
#include "chef_base/chef_snippet.hpp"

namespace yet {

class Config {
  public:
    static Config *instance();

    // just for mem check
    static void dispose();

  public:
    CHEF_PROPERTY(std::string, http_flv_pull_host);

  private:
    Config() {}

  private:
    Config(const Config &) = delete;
    Config &operator=(const Config &) = delete;

  private:
    static Config *core_;
};

}
