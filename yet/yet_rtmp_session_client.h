/**
 * @file   yet_rtmp_session_client.h
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

// RtmpSessionClient means that this session is initiating a tcp connection to peer.
// Push means that the av data flow from local to peer.
// Pull means that the av data flow peer to local.
class RtmpSessionClient : public RtmpSessionBase {
  public:
    static std::shared_ptr<RtmpSessionClient> create_push(asio::io_context &io_ctx);
    static std::shared_ptr<RtmpSessionClient> create_pull(asio::io_context &io_ctx);

    void async_start(const std::string url);
    void async_start(const std::string &peer_ip, uint16_t peer_port, const std::string &app_name, const std::string &stream_name);

    // CHEFTODO push pull start, succ cb

    void dispose() {}

  private:
    void do_tcp_connect();
    void tcp_connect_handler(const ErrorCode &ec, const asio::ip::tcp::endpoint &endpoint);
    void do_write_c0c1();
    void do_read_s0s1s2();
    void do_write_c2();
    void do_write_chunk_size();
    void do_write_rtmp_connect();

  private:
    virtual void on_command_message(const std::string &cmd, uint32_t tid, uint8_t *pos, size_t len) override;

  private:
    void cmd_msg_result_handler(uint8_t *pos, size_t len, uint32_t tid);
    void cmd_msg_on_status_handler(uint8_t *pos, size_t len, uint32_t tid);

  private:
    void do_write_create_stream();
    void do_write_play();
    void do_write_publish();

  private:
    RtmpSessionClientPtr get_self();

  public:
    virtual ~RtmpSessionClient();

  private:
    RtmpSessionClient(asio::io_context &io_ctx, RtmpSessionType type);

    RtmpSessionClient(const RtmpSessionClient &) = delete;
    RtmpSessionClient &operator=(const RtmpSessionClient &) = delete;

  private:
    asio::ip::tcp::resolver resolver_;
    Buffer                  write_buf_;
    std::string             peer_ip_;
    uint16_t                peer_port_;
    RtmpHandshakeC          handshake_;
    uint32_t                stream_id_;
};

}
