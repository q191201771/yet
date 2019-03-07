#include "yet_rtmp_server.h"
#include <asio.hpp>
#include "yet.hpp"
#include "yet_server.h"
#include "yet_group.h"
#include "yet_rtmp_session_pub_sub.h"

namespace yet {

RtmpServer::RtmpServer(asio::io_context &io_ctx, const std::string &listen_ip, uint16_t listen_port, Server *server)
  : io_ctx_(io_ctx)
  , listen_ip_(listen_ip)
  , listen_port_(listen_port)
  , server_(server)
  , acceptor_(io_ctx_, asio::ip::tcp::endpoint(asio::ip::address_v4::from_string(listen_ip_), listen_port_))
{
  YET_LOG_DEBUG("[{}] [lifecycle] new RtmpServer.", static_cast<void *>(this));
}

RtmpServer::~RtmpServer() {
  YET_LOG_DEBUG("[{}] [lifecycle] delete RtmpServer.", static_cast<void *>(this));
}

void RtmpServer::do_accept() {
  acceptor_.async_accept(std::bind(&RtmpServer::accept_cb, this, _1, _2));
}

void RtmpServer::accept_cb(ErrorCode ec, asio::ip::tcp::socket socket) {
  if (ec) {
    YET_LOG_ERROR("rtmp server accept failed. ec:{}", ec.message());
    return;
  }

  auto session = std::make_shared<RtmpSessionPubSub>(std::move(socket));
  session->set_pub_start_cb(std::bind(&RtmpServer::on_pub_start, this, _1));
  session->set_sub_start_cb(std::bind(&RtmpServer::on_sub_start, this, _1));
  session->set_pub_stop_cb(std::bind(&RtmpServer::on_pub_stop, this, _1));
  session->set_rtmp_session_close_cb(std::bind(&RtmpServer::on_rtmp_session_close, this, _1));
  session->start();

  do_accept();
}

void RtmpServer::start() {
  YET_LOG_INFO("start rtmp server. {}:{}", listen_ip_, listen_port_);
  do_accept();
}

void RtmpServer::dispose() {
  YET_LOG_INFO("dispose rtmp server.");
  acceptor_.close();
}

void RtmpServer::on_pub_start(RtmpSessionPubSubPtr session) {
  auto group = server_->get_or_create_group(session->app_name(), session->stream_name());
  group->on_rtmp_pub_start(session);
}

void RtmpServer::on_sub_start(RtmpSessionPubSubPtr session) {
  auto group = server_->get_or_create_group(session->app_name(), session->stream_name());
  group->add_rtmp_sub(session);
}

void RtmpServer::on_pub_stop(RtmpSessionPubSubPtr session) {
  auto group = server_->get_group(session->stream_name());
  if (!group) {
    YET_LOG_WARN("group not exist while publish stop. {}", session->stream_name());
  }
  group->on_rtmp_pub_stop();
}

void RtmpServer::on_rtmp_session_close(RtmpSessionBasePtr session) {
  auto group = server_->get_group(session->stream_name());
  if (!group) {
    YET_LOG_DEBUG("group not exist while session close. {}", session->stream_name());
    return;
  }
  group->on_rtmp_session_close(session);
}

}
