#include "yet_config.h"
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include "yet_common/yet_log.h"

using json = nlohmann::json;

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

#define SNIPPET_ENSURE_ATTR_EXIST(node, attr) \
  if (node[attr].is_null()) { \
    YET_LOG_ERROR("load conf file failed since attr not exist. {}", attr); \
    return false; \
  }

#define SNIPPET_ENSURE_ATTR_TYPE_EXIST(node, attr, type) \
  SNIPPET_ENSURE_ATTR_EXIST(node, attr) \
  if (!node[attr].is_##type()) { \
    YET_LOG_ERROR("load conf file failed since attr type not {}. {}", #type, attr); \
  }

#define SNIPPET_ENSURE_ATTR_STRING_EXIST(node, attr) SNIPPET_ENSURE_ATTR_TYPE_EXIST(node, attr, string)
#define SNIPPET_ENSURE_ATTR_NUMBER_EXIST(node, attr) SNIPPET_ENSURE_ATTR_TYPE_EXIST(node, attr, number)

bool Config::load_conf_file(const std::string &filename) {
  try {
    std::ifstream i(filename.c_str());
    json j;
    i >> j;
    YET_LOG_INFO("conf file content: {}", j.dump());

    SNIPPET_ENSURE_ATTR_STRING_EXIST(j, "rtmp_server_ip");
    rtmp_server_ip_ = j["rtmp_server_ip"];
    SNIPPET_ENSURE_ATTR_NUMBER_EXIST(j, "rtmp_server_port");
    rtmp_server_port_ = j["rtmp_server_port"];
    SNIPPET_ENSURE_ATTR_STRING_EXIST(j, "http_flv_server_ip");
    http_flv_server_ip_ = j["http_flv_server_ip"];
    SNIPPET_ENSURE_ATTR_NUMBER_EXIST(j, "http_flv_server_port");
    http_flv_server_port_ = j["http_flv_server_port"];

    if (j["rtmp_pull_host"].is_string()) {
      rtmp_pull_host_ = j["rtmp_pull_host"];
      pull_rtmp_if_stream_not_exist_ = true;
    }

    if (j["rtmp_pull_port"].is_number()) {
      rtmp_pull_port_ = j["rtmp_pull_port"];
    }

    if (j["rtmp_push_host"].is_string()) {
      rtmp_push_host_ = j["rtmp_push_host"];
      push_rtmp_if_pub_ = true;
    }

    if (j["rtmp_push_port"].is_number()) {
      rtmp_push_port_ = j["rtmp_push_port"];
    }

  } catch(...) {
    // file not exist or syntax invalid
    YET_LOG_ERROR("load conf file failed. {}", filename);
    return false;
  }
  return true;
}

}
