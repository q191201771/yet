/**
 * @file   yet_config.h
 * @author pengrl
 *
 */

#pragma once

#include <string>
#include "chef_base/chef_snippet.hpp"

class Config {
  public:
    //CHEF_PROPERTY_WITH_INIT_VALUE(uint16_t, rtmp_port, 0);
    //CHEF_PROPERTY_WITH_INIT_VALUE(uint16_t, http_flv_port, 0);
    CHEF_PROPERTY(std::string, http_flv_pull_host);

  public:
    Config() {}
  private:
    Config(const Config &) = delete;
    Config &operator=(const Config &) = delete;
};
