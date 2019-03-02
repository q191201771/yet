#include "yet_server.h"
#include <asio.hpp>
#include "yet_rtmp_server.h"
#include "yet_http_flv_server.h"
#include "yet_group.h"
#include "yet_config.h"
// CHEFTODO erase me
#include "yet_rtmp_pull.h"

namespace yet {

Server::Server(const std::string &rtmp_listen_ip, uint16_t rtmp_listen_port,
               const std::string &http_flv_listen_ip, uint16_t http_flv_listen_port)
  : rtmp_server_(std::make_shared<RtmpServer>(io_ctx_, rtmp_listen_ip, rtmp_listen_port, this))
  , http_flv_server_(std::make_shared<HttpFlvServer>(io_ctx_, http_flv_listen_ip, http_flv_listen_port, this))
{
  YET_LOG_DEBUG("Server(). {}", (void *)this);
}

Server::~Server() {
  YET_LOG_DEBUG("~Server(). {}", (void *)this);
}

void Server::run_loop() {
  auto rp = std::make_shared<RtmpPull>(io_ctx_);
  rp->async_pull(Config::instance()->rtmp_pull_host(), 1935, "live", "110");

  rtmp_server_->start();
  http_flv_server_->start();
  io_ctx_.run();
}

void Server::dispose() {
  rtmp_server_->dispose();
  http_flv_server_->dispose();

  for (auto &it : live_name_2_group_) {
    it.second->dispose();
  }

  io_ctx_.stop();
}

GroupPtr Server::get_or_create_group(const std::string &live_name) {
  auto iter = live_name_2_group_.find(live_name);
  if (iter != live_name_2_group_.end()) {
    return iter->second;
  }

  auto group = std::make_shared<Group>(live_name);
  live_name_2_group_[live_name] = group;
  return group;
}

GroupPtr Server::get_group(const std::string &live_name) {
  auto iter = live_name_2_group_.find(live_name);
  if (iter != live_name_2_group_.end()) {
    return iter->second;
  }
  return nullptr;
}

}
