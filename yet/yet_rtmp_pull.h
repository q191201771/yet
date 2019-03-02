/**
 * @file   yet_rtmp_pull.h
 * @author pengrl
 *
 */

#pragma once

#include <memory>
#include <asio.hpp>
#include "yet_rtmp/yet_rtmp_handshake.h"
#include "yet.hpp"
#include "yet_rtmp_base.h"

namespace yet {

class RtmpPull : public RtmpSessionBase {
  public:
    RtmpPull(asio::io_context &io_context);
    ~RtmpPull();

    void async_pull(const std::string url);
    void async_pull(const std::string &peer_ip, uint16_t peer_port, const std::string &app_name, const std::string &live_name);

  private:
    void do_tcp_connect();
    void tcp_connect_cb(const ErrorCode &ec, const asio::ip::tcp::endpoint &endpoint);
    void do_write_c0c1();
    void do_read_s0s1s2();
    void do_write_c2();
    void do_write_chunk_size();
    void do_write_rtmp_connect();

  private:
    virtual void rtmp_connect_result_handler();
    virtual void create_stream_result_handler();
    virtual void play_result_handler();

  private:
    RtmpPullPtr get_self();

  private:
    asio::ip::tcp::resolver resolver_;
    Buffer                  write_buf_;
    std::string             peer_ip_;
    uint16_t                peer_port_;
    std::string             app_name_;
    std::string             live_name_;
    RtmpHandshakeC          handshake_;
};

}
