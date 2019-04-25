#include "yet_rtmp_server.h"
#include <asio.hpp>
#include "yet.hpp"
#include "yet_server.h"
#include "yet_group.h"
#include "yet_rtmp_session_server.h"

namespace yet {

RtmpServer::RtmpServer(asio::io_context &io_ctx, const std::string &listen_ip, uint16_t listen_port, Server *server)
  : io_ctx_(io_ctx)
  , listen_ip_(listen_ip)
  , listen_port_(listen_port)
  , server_(server)
{
  YET_LOG_DEBUG("[{}] [lifecycle] new RtmpServer.", static_cast<void *>(this));
}

RtmpServer::~RtmpServer() {
  YET_LOG_DEBUG("[{}] [lifecycle] delete RtmpServer.", static_cast<void *>(this));
}

void RtmpServer::do_accept() {
  acceptor_->async_accept([this](const ErrorCode &ec, asio::ip::tcp::socket socket) {
                           YET_LOG_ASSERT(!ec, "rtmp server accept failed. ec:{}", ec.message());
                           auto session = std::make_shared<RtmpSessionServer>(std::move(socket));
                           session->set_pub_start_cb([this](auto sessionPtr) { on_pub_start(sessionPtr); });
                           session->set_sub_start_cb([this](auto sessionPtr) { on_sub_start(sessionPtr); });
                           session->set_pub_stop_cb([this](auto sessionPtr) { on_pub_stop(sessionPtr); });
                           session->set_rtmp_session_close_cb([this](auto sessionPtr) { on_rtmp_session_close(sessionPtr); });
                           session->start();
                           do_accept();
                         });
}

bool RtmpServer::start() {
  YET_LOG_INFO("start rtmp server. {}:{}", listen_ip_, listen_port_);
  try {
    acceptor_ = std::unique_ptr<asio::ip::tcp::acceptor>(new asio::ip::tcp::acceptor(io_ctx_,
        asio::ip::tcp::endpoint(asio::ip::address_v4::from_string(listen_ip_), listen_port_)));
  } catch (const std::system_error &err) {
    YET_LOG_ERROR("rtmp server listen failed. ip:{}, port:{}, ec:{}", listen_ip_, listen_port_, err.code().message());
    return false;
  }
  do_accept();
  return true;
}

void RtmpServer::dispose() {
  YET_LOG_INFO("dispose rtmp server.");
  acceptor_->close();
}

void RtmpServer::on_pub_start(RtmpSessionServerPtr session) {
  auto group = server_->get_or_create_group(session->app_name(), session->stream_name());
  group->on_rtmp_pub_start(session);
}

void RtmpServer::on_sub_start(RtmpSessionServerPtr session) {
  auto group = server_->get_or_create_group(session->app_name(), session->stream_name());
  group->add_rtmp_sub(session);
}

void RtmpServer::on_pub_stop(RtmpSessionServerPtr session) {
  auto group = server_->get_group(session->stream_name());
  YET_LOG_ASSERT(group, "group not exist while publish stop. {}", session->stream_name());
  group->on_rtmp_pub_stop();
}

void RtmpServer::on_rtmp_session_close(RtmpSessionBasePtr session) {
  auto group = server_->get_group(session->stream_name());
  YET_LOG_ASSERT(group, "group not exist while session close. {}", session->stream_name());
  group->on_rtmp_session_close(session);
}

}
