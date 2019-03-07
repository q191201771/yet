/**
 * @file   yet_config.h
 * @author pengrl
 *
 */

#pragma once

#include <string>
#include "chef_base/chef_snippet.hpp"

namespace yet {

class Config {
  public:
    static Config *instance();

    bool load_conf_file(const std::string &filename);

    // just for mem check
    static void dispose();

  public:
    CHEF_PROPERTY(std::string, rtmp_server_ip);
    CHEF_PROPERTY(uint16_t, rtmp_server_port);

    CHEF_PROPERTY(std::string, http_flv_server_ip);
    CHEF_PROPERTY(uint16_t, http_flv_server_port);

    CHEF_PROPERTY(std::string, rtmp_pull_host);
    CHEF_PROPERTY_WITH_INIT_VALUE(uint16_t, rtmp_pull_port, 1935);
    CHEF_PROPERTY_WITH_INIT_VALUE(bool, pull_rtmp_if_stream_not_exist, false);

    CHEF_PROPERTY(std::string, rtmp_push_host);
    CHEF_PROPERTY_WITH_INIT_VALUE(uint16_t, rtmp_push_port, 1935);
    CHEF_PROPERTY_WITH_INIT_VALUE(bool, push_rtmp_if_pub, false);

  private:
    Config() {}

  private:
    Config(const Config &) = delete;
    Config &operator=(const Config &) = delete;

  private:
    static Config *core_;
};

}
