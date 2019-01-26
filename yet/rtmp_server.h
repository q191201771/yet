/**
 * @file   rtmp_server.h
 * @author pengrl
 *
 */

#pragma once

#include <string>
#include <yet.hpp>
#include "rtmp_session.h"

namespace yet {

class RtmpServer : public std::enable_shared_from_this<RtmpServer>
                 , public RtmpSessionObserver
{
  public:
    RtmpServer(asio::io_context &io_ctx, const std::string &listen_ip, uint16_t listen_port, Server *server);

    void start();
    void dispose();

  private:
    virtual void on_rtmp_publish(RtmpSessionPtr session, const std::string &app, const std::string &live_name);
    virtual void on_rtmp_play(RtmpSessionPtr session, const std::string &app, const std::string &live_name);

  private:
    void do_accept();
    void accept_cb(ErrorCode ec, asio::ip::tcp::socket socket);

  private:
    RtmpServer(const RtmpServer &) = delete;
    RtmpServer &operator=(const RtmpServer &) = delete;

  private:
    asio::io_context        &io_ctx_;
    std::string             listen_ip_;
    uint16_t                listen_port_;
    Server                  *server_;
    asio::ip::tcp::acceptor acceptor_;
};

}
