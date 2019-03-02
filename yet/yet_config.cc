#include "yet_config.h"
#include "yet_common/yet_log.h"
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

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

    if (j["http_flv_pull_host"].is_string()) {
      http_flv_pull_host_ = j["http_flv_pull_host"];
    }

    if (j["rtmp_pull_host"].is_string()) {
      rtmp_pull_host_ = j["rtmp_pull_host"];
    }

  } catch(...) {
    // file not exist or syntax invalid
    YET_LOG_ERROR("load conf file failed. {}", filename);
    return false;
  }
  return true;
}

}
