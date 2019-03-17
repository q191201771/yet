#include "yet_server.h"
#include <asio.hpp>
#include "yet_config.h"
#include "yet_http_flv_server.h"
#include "yet_rtmp_server.h"
#include "yet_group.h"

namespace yet {

static constexpr size_t CRUDE_TIMER_INTERVAL_MSEC = 1000;

Server::Server(const std::string &rtmp_listen_ip, uint16_t rtmp_listen_port,
               const std::string &http_flv_listen_ip, uint16_t http_flv_listen_port)
  : crude_timer_(io_ctx_, asio::chrono::milliseconds(CRUDE_TIMER_INTERVAL_MSEC))
  , rtmp_server_(std::make_shared<RtmpServer>(io_ctx_, rtmp_listen_ip, rtmp_listen_port, this))
  , http_flv_server_(std::make_shared<HttpFlvServer>(io_ctx_, http_flv_listen_ip, http_flv_listen_port, this))
{
  YET_LOG_DEBUG("[{}] [lifecycle] new Server.", (void *)this);
}

Server::~Server() {
  YET_LOG_DEBUG("[{}] [lifecycle] delete Server.", (void *)this);
}

void Server::run_loop() {

  rtmp_server_->start();
  http_flv_server_->start();
  do_crude_timer();
  io_ctx_.run();
}

void Server::dispose() {
  rtmp_server_->dispose();
  http_flv_server_->dispose();

  for (auto &it : stream_name_2_group_) {
    it.second->dispose();
  }

  io_ctx_.stop();
}

void Server::do_crude_timer() {
  if (!crude_timer_first_) {
    crude_timer_.expires_at(crude_timer_.expires_at() + asio::chrono::milliseconds(CRUDE_TIMER_INTERVAL_MSEC));
  }
  crude_timer_.async_wait(std::bind(&Server::crude_timer_handler, shared_from_this(), _1));
}

void Server::crude_timer_handler(const ErrorCode &ec) {
  if (ec) {
    YET_LOG_ERROR("crude timer error. ec:{}", ec.message());
    return;
  }

  if (crude_timer_tick_count_msec_ % CLEAN_EMPTY_GROUP_INTERVAL_MSEC == 0){
    auto iter = stream_name_2_group_.begin();
    for (; iter != stream_name_2_group_.end(); ) {
      if (iter->second->empty_totally()) {
        YET_LOG_DEBUG("erase empty group. {}", iter->second->stream_name());
        iter = stream_name_2_group_.erase(iter);
      } else {
        iter++;
      }
    }
  }

  crude_timer_tick_count_msec_ += CRUDE_TIMER_INTERVAL_MSEC;
  crude_timer_first_ = false;
  do_crude_timer();
}

GroupPtr Server::get_or_create_group(const std::string &app_name, const std::string &stream_name) {
  // CHEFTODO manage app_name with group
  auto iter = stream_name_2_group_.find(stream_name);
  if (iter != stream_name_2_group_.end()) {
    return iter->second;
  }

  auto group = std::make_shared<Group>(io_ctx_, app_name, stream_name);
  stream_name_2_group_[stream_name] = group;
  return group;
}

GroupPtr Server::get_group(const std::string &stream_name) {
  auto iter = stream_name_2_group_.find(stream_name);
  if (iter != stream_name_2_group_.end()) {
    return iter->second;
  }
  return nullptr;
}

}
