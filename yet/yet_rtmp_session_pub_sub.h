/**
 * @file   yet_rtmp_session_pub_sub.h
 * @author pengrl
 *
 */

#pragma once

#include <memory>
#include <unordered_map>
#include <asio.hpp>
#include "chef_base/chef_snippet.hpp"
#include "yet_rtmp/yet_rtmp_chunk_op.h"
#include "yet_rtmp/yet_rtmp.hpp"
#include "yet_rtmp/yet_rtmp_handshake.h"
#include "yet.hpp"
#include "yet_rtmp_session_base.h"

namespace yet {

// RtmpSessionPubSub means that this session is accepting incoming tcp connection which initiated by peer.
// Pub means that the av data flow from peer to local.
// Sub means that the av data flow local to peer.
class RtmpSessionPubSub : public RtmpSessionBase {
  public:
    explicit RtmpSessionPubSub(asio::ip::tcp::socket socket);
    virtual ~RtmpSessionPubSub();

  public:
    using RtmpPubSubEventCb = std::function<void(RtmpSessionPubSubPtr session)>;

    void set_pub_start_cb(RtmpPubSubEventCb cb); // only for pub session while recv rtmp publish command message
    void set_sub_start_cb(RtmpPubSubEventCb cb); // only for sub session while recv rtmp play command message
    void set_pub_stop_cb(RtmpPubSubEventCb cb); // only for pub session while recv rtmp command message

    void start();

    void dispose() {}

  private:
    virtual void on_command_message(const std::string &cmd, uint32_t tid, uint8_t *pos, size_t len) override;

  private:
    void do_read_c0c1();
    void do_write_s0s1();
    void do_write_s2();
    void do_read_c2();

  private:
    void connect_handler(uint32_t tid, uint8_t *buf, size_t len);
    void create_stream_handler(uint32_t tid, uint8_t *buf, size_t len);
    void publish_handler(uint32_t tid, uint8_t *buf, size_t len);
    void play_handler(uint32_t tid, uint8_t *buf, size_t len);
    void delete_stream_handler(uint32_t tid, uint8_t *buf, size_t len);

  private:
    void do_write_win_ack_size();
    void do_write_peer_bandwidth();
    void do_write_chunk_size();
    void do_write_connect_result();
    void do_write_create_stream_result();
    void do_write_on_status_publish();
    void do_write_on_status_play();

  private:
    RtmpSessionPubSubPtr get_self();

  private:
    RtmpSessionPubSub(const RtmpSessionPubSub &) = delete;
    RtmpSessionPubSub &operator=(const RtmpSessionPubSub &) = delete;

  private:
    RtmpHandshakeS    handshake_;
    chef::buffer      write_buf_;
    uint32_t          curr_tid_;
    RtmpPubSubEventCb pub_start_cb_;
    RtmpPubSubEventCb sub_start_cb_;
    RtmpPubSubEventCb pub_stop_cb_;
};

}
