/**
 * @file   yet_rtmp_server.h
 * @author pengrl
 * @date   20190127
 *
 */

#pragma once

#include <string>
#include <asio.hpp>
#include "yet.hpp"
#include "yet_rtmp_session.h"

namespace yet {

class RtmpServer : public std::enable_shared_from_this<RtmpServer>
{
  public:
    RtmpServer(asio::io_context &io_ctx, const std::string &listen_ip, uint16_t listen_port, Server *server);
    ~RtmpServer();

    void start();
    void dispose();

  private:
    void on_rtmp_publish(RtmpSessionPtr session);
    void on_rtmp_play(RtmpSessionPtr session);
    void on_rtmp_publish_stop(RtmpSessionPtr session);
    void on_rtmp_session_close(RtmpSessionPtr session);

  private:
    void do_accept();
    void accept_cb(ErrorCode ec, asio::ip::tcp::socket socket);

  private:
    RtmpServer(const RtmpServer &) = delete;
    RtmpServer &operator=(const RtmpServer &) = delete;

  private:
    asio::io_context        &io_ctx_;
    const std::string       listen_ip_;
    const uint16_t          listen_port_;
    Server                  *server_;
    asio::ip::tcp::acceptor acceptor_;
};

}
