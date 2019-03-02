/**
 * @file   yet_rtmp_base.h
 * @author pengrl
 *
 */

#pragma once

#include <asio.hpp>
#include "yet.hpp"
#include "yet_rtmp/yet_rtmp.hpp"
#include "yet_rtmp/yet_rtmp_protocol.h"

namespace yet {

enum class RtmpSessionBaseType {
  RTMP_SESSION_TYPE_UNKNOWN = 0,
  RTMP_SESSION_TYPE_PULL
};

class RtmpSessionBase : public std::enable_shared_from_this<RtmpSessionBase> {
  public:
    RtmpSessionBase(asio::io_context &io_context, RtmpSessionBaseType type);
    RtmpSessionBase(asio::ip::tcp::socket socket);
    virtual ~RtmpSessionBase() {}

    void do_read();
    virtual void rtmp_connect_result_handler() = 0;
    virtual void create_stream_result_handler() = 0;
    virtual void play_result_handler() = 0;

    virtual void complete_message_handler(RtmpStreamPtr stream);

    virtual void protocol_control_message_handler();
    virtual void ctrl_msg_win_ack_size_handler(int val);
    virtual void ctrl_msg_set_chunk_size_handler(int val);

    virtual void command_message_handler();
    virtual void cmd_msg_result_handler(uint8_t *pos, std::size_t len, uint32_t tid);
    virtual void cmd_msg_on_status_handler(uint8_t *pos, std::size_t len, uint32_t tid);

    virtual void av_handler();

  protected:
    // CHEFTODO
    void close() {}

  private:
    RtmpSessionBase(const RtmpSessionBase &) = delete;
    RtmpSessionBase &operator=(const RtmpSessionBase &) = delete;

  protected:
    asio::ip::tcp::socket socket_;
    RtmpSessionBaseType   type_;
    Buffer                read_buf_;
    uint32_t              stream_id_;

  private:
    RtmpProtocol          rtmp_protocol_;
    RtmpStreamPtr         curr_stream_; // CHEFTODO erase me
    int                   peer_win_ack_size_ = -1;
};

}
