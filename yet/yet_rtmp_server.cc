#include "yet_rtmp_server.h"
#include <asio.hpp>
#include "yet_server.h"
#include "yet_group.h"
#include "yet_rtmp_session.h"
#include "yet.hpp"

namespace yet {

RtmpServer::RtmpServer(asio::io_context &io_ctx, const std::string &listen_ip, uint16_t listen_port, Server *server)
  : io_ctx_(io_ctx)
  , listen_ip_(listen_ip)
  , listen_port_(listen_port)
  , server_(server)
  , acceptor_(io_ctx_, asio::ip::tcp::endpoint(asio::ip::address_v4::from_string(listen_ip_), listen_port_))
{
  YET_LOG_DEBUG("RtmpServer() {}.", static_cast<void *>(this));
}

RtmpServer::~RtmpServer() {
  YET_LOG_DEBUG("~RtmpServer() {}.", static_cast<void *>(this));
}

void RtmpServer::do_accept() {
  acceptor_.async_accept(std::bind(&RtmpServer::accept_cb, this, _1, _2));
}

void RtmpServer::accept_cb(ErrorCode ec, asio::ip::tcp::socket socket) {
  if (ec) {
    YET_LOG_ERROR("Accept failed. ec:{}", ec.message());
    return;
  }

  auto session = std::make_shared<RtmpSession>(std::move(socket));
  session->set_rtmp_publish_cb(std::bind(&RtmpServer::on_rtmp_publish, this, _1));
  session->set_rtmp_play_cb(std::bind(&RtmpServer::on_rtmp_play, this, _1));
  session->set_rtmp_publish_stop_cb(std::bind(&RtmpServer::on_rtmp_publish_stop, this, _1));
  session->start();

  do_accept();
}

void RtmpServer::start() {
  YET_LOG_INFO("Start rtmp server. port:{}", listen_port_);
  do_accept();
}

void RtmpServer::dispose() {
  YET_LOG_INFO("Stop rtmp server listen.");
  acceptor_.close();
}

void RtmpServer::on_rtmp_publish(RtmpSessionPtr session) {
  auto group = server_->get_or_create_group(session->live_name());
  group->set_rtmp_pub(session);
}

void RtmpServer::on_rtmp_play(RtmpSessionPtr session) {
  auto group = server_->get_or_create_group(session->live_name());
  group->add_rtmp_sub(session);
  group->on_rtmp_publish();
}

void RtmpServer::on_rtmp_publish_stop(RtmpSessionPtr session) {
  auto group = server_->get_group(session->live_name());
  if (!group) {
    YET_LOG_WARN("Group not exist while publish stop. {}", session->live_name());
  }
  group->on_rtmp_publish_stop();
}

void RtmpServer::on_rtmp_session_close(RtmpSessionPtr session) {
  auto group = server_->get_group(session->live_name());
  if (!group) {
    YET_LOG_WARN("Group not exist while session close. {}", session->live_name());
  }
  group->on_rtmp_session_close(session);
}

}
