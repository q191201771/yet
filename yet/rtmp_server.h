/**
 * @file   rtmp_server.h
 * @author pengrl
 *
 */

#pragma once

#include <cinttypes>
#include <string>
#include <asio.hpp>

namespace yet {

class RTMPServer {
  public:
    RTMPServer(const std::string &listen_ip, uint16_t listen_port);

    void run_loop();

  private:
    void do_accept();
    void accept_cb(asio::error_code ec, asio::ip::tcp::socket socket);

  private:
    RTMPServer(const RTMPServer &) = delete;
    RTMPServer &operator=(const RTMPServer &) = delete;

  private:
    std::string             listen_ip_;
    uint16_t                listen_port_;
    asio::io_context        io_ctx_;
    asio::ip::tcp::acceptor acceptor_;
};

}
