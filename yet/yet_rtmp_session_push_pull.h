/**
 * @file   yet_rtmp_session_push_pull.h
 * @author pengrl
 *
 */

#pragma once

#include <memory>
#include <asio.hpp>
#include "yet_rtmp/yet_rtmp_handshake.h"
#include "yet.hpp"
#include "yet_rtmp_session_base.h"

namespace yet {

class RtmpSessionPushPull : public RtmpSessionBase {
  public:
    static std::shared_ptr<RtmpSessionPushPull> create_push(asio::io_context &io_ctx);
    static std::shared_ptr<RtmpSessionPushPull> create_pull(asio::io_context &io_ctx);

    void async_start(const std::string url);
    void async_start(const std::string &peer_ip, uint16_t peer_port, const std::string &app_name, const std::string &stream_name);

    // CHEFTODO push pull start, succ cb

  private:
    void do_tcp_connect();
    void tcp_connect_handler(const ErrorCode &ec, const asio::ip::tcp::endpoint &endpoint);
    void do_write_c0c1();
    void do_read_s0s1s2();
    void do_write_c2();
    void do_write_chunk_size();
    void do_write_rtmp_connect();

  private:
    virtual void on_command_message(const std::string &cmd, uint32_t tid, uint8_t *pos, std::size_t len);

  private:
    void cmd_msg_result_handler(uint8_t *pos, std::size_t len, uint32_t tid);
    void cmd_msg_on_status_handler(uint8_t *pos, std::size_t len, uint32_t tid);

  private:
    void do_write_release_stream();
    void do_write_create_stream(uint32_t tid);
    void do_write_play();
    void do_write_publish();

  private:
    RtmpSessionPushPullPtr get_self();

  public:
    virtual ~RtmpSessionPushPull();

  private:
    RtmpSessionPushPull(asio::io_context &io_ctx, RtmpSessionType type);

    RtmpSessionPushPull(const RtmpSessionPushPull &) = delete;
    RtmpSessionPushPull &operator=(const RtmpSessionPushPull &) = delete;

  private:
    asio::ip::tcp::resolver resolver_;
    Buffer                  write_buf_;
    std::string             peer_ip_;
    uint16_t                peer_port_;
    RtmpHandshakeC          handshake_;
    uint32_t                stream_id_;
};

}
