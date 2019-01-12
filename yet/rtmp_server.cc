#include "rtmp_server.h"
#include "rtmp_session.h"
#include "yet_inner.hpp"

namespace yet {

RTMPServer::RTMPServer(const std::string &listen_ip, uint16_t listen_port)
  : listen_ip_(listen_ip)
  , listen_port_(listen_port)
  , acceptor_(io_ctx_, asio::ip::tcp::endpoint(asio::ip::address_v4::from_string(listen_ip_), listen_port_))
{
}

void RTMPServer::do_accept() {
  acceptor_.async_accept(std::bind(&RTMPServer::accept_cb, this, _1, _2));
}

void RTMPServer::accept_cb(ErrorCode ec, asio::ip::tcp::socket socket) {
  YET_LOG_DEBUG("accept_cb ec:{}", ec.message());

  if (!ec) {
    auto session = std::make_shared<RTMPSession>(std::move(socket));
    session->start();
  } else {
    YET_LOG_ERROR("Accept failed. ec:{}", ec.message());
  }

  do_accept();
}

void RTMPServer::run_loop() {
  do_accept();
  io_ctx_.run();
}

}
