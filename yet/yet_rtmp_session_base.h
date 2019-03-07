/**
 * @file   yet_session_rtmp_base.h
 * @author pengrl
 *
 */

#pragma once

#include <queue>
#include <asio.hpp>
#include "chef_base/chef_snippet.hpp"
#include "yet_rtmp/yet_rtmp.hpp"
#include "yet_rtmp/yet_rtmp_protocol.h"
#include "yet.hpp"

namespace yet {

enum class RtmpSessionType {
  INVALID = 0,
  PUB_SUB = 1, // while accept a connection at rtmp port, we still don't know this session whether pub or sub,
               // and will mod its type to pub or sub until recv rtmp publish/play command message
  PUB,
  SUB,
  PUSH,
  PULL
};

class RtmpSessionBase : public std::enable_shared_from_this<RtmpSessionBase> {
  public:
    RtmpSessionBase(asio::io_context &io_ctx, RtmpSessionType type);
    RtmpSessionBase(asio::ip::tcp::socket socket, RtmpSessionType type);
    virtual ~RtmpSessionBase() {}

  public:
    using RtmpEventCb = std::function<void(RtmpSessionBasePtr session)>;
    using RtmpMetaDataCb = std::function<void(RtmpSessionBasePtr session, BufferPtr buf, uint8_t *meta_pos,
                                              std::size_t meta_size, AmfObjectItemMapPtr)>;
    using RtmpAvDataCb = std::function<void(RtmpSessionBasePtr session, BufferPtr buf, const RtmpHeader &header)>;

    void set_rtmp_session_close_cb(RtmpEventCb cb); // for pub & sub & push & pull session
    void set_rtmp_meta_data_cb(RtmpMetaDataCb cb);  // for pub & pull session
    void set_rtmp_av_data_cb(RtmpAvDataCb cb);      // for pub & pull session

    void async_send(BufferPtr buf); // for sub & push session

    RtmpSessionType type();
    RtmpSessionPubSubPtr cast_to_pub_sub(); // for pub & sub session
    RtmpSessionPushPullPtr cast_to_push_pull(); // for pub & sub session

  protected:
    virtual void on_command_message(const std::string &cmd, uint32_t tid, uint8_t *pos, std::size_t len) = 0;

  protected:
    void do_read();
    void close();

  private:
    void base_complete_message_handler(RtmpStreamPtr stream);
    void base_protocol_control_message_handler();
    void base_command_message_handler();
    void base_ctrl_msg_win_ack_size_handler(int val);
    void base_ctrl_msg_set_chunk_size_handler(int val);
    void base_user_control_message_handler();
    void base_av_data_handler();
    void base_meta_data_handler();

  private:
    void do_send();

  private:
    RtmpSessionBase(const RtmpSessionBase &) = delete;
    RtmpSessionBase &operator=(const RtmpSessionBase &) = delete;

  protected:
    asio::ip::tcp::socket socket_;
    RtmpSessionType       type_;
    Buffer                read_buf_;
    RtmpStreamPtr         curr_stream_; // CHEFTODO erase me

    // for sub & push
    CHEF_PROPERTY_WITH_INIT_VALUE(bool, has_sent_metadata, false);
    CHEF_PROPERTY_WITH_INIT_VALUE(bool, has_sent_audio, false);
    CHEF_PROPERTY_WITH_INIT_VALUE(bool, has_sent_video, false);
    CHEF_PROPERTY_WITH_INIT_VALUE(bool, has_sent_key_frame, false);

    // for push
    CHEF_PROPERTY_WITH_INIT_VALUE(bool, is_ready, false);

  private:
    RtmpProtocol          rtmp_protocol_;
    int                   peer_win_ack_size_ = -1;
    RtmpEventCb           rtmp_session_close_cb_;
    RtmpMetaDataCb        rtmp_meta_data_cb_;
    RtmpAvDataCb          rtmp_av_data_cb_;
    std::queue<BufferPtr> send_buffers_;

  private:
    CHEF_PROPERTY(std::string, app_name);
    CHEF_PROPERTY(std::string, stream_name);
};

#define SNIPPET_RTMP_SESSION_ENTER_CB \
  if (ec) { \
    YET_LOG_ERROR("[{}] ec:{}", (void *)this, ec.message()); \
    if (ec == asio::error::eof) { \
      YET_LOG_INFO("[{}] close by peer.", (void *)this); \
      close(); \
    } else if (ec == asio::error::broken_pipe) { \
      YET_LOG_INFO("[{}] broken pipe.", (void *)this); \
    } \
    return; \
  } \

}
