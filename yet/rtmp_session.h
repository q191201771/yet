/**
 * @file   rtmp_session.h
 * @author pengrl
 * @date   20190127
 *
 */

#pragma once

#include <memory>
#include <queue>
#include <asio.hpp>
#include "yet.hpp"
#include "chef_base/chef_snippet.hpp"
#include "yet_rtmp/rtmp.hpp"
#include "yet_rtmp/rtmp_handshake.h"

namespace yet {

enum RtmpSessionType {
  RTMP_SESSION_TYPE_UNKNOWN = 1,
  RTMP_SESSION_TYPE_PUB     = 2,
  RTMP_SESSION_TYPE_SUB     = 3
};

class RtmpSessionObserver {
  public:
    virtual ~RtmpSessionObserver() {}
    //virtual void on_rtmp_connected() = 0;

    virtual void on_rtmp_publish(RtmpSessionPtr session, const std::string &app, const std::string &live_name) = 0;

    virtual void on_rtmp_play(RtmpSessionPtr session, const std::string &app, const std::string &live_name) = 0;
};

class RtmpSession : public std::enable_shared_from_this<RtmpSession> {
  public:
    explicit RtmpSession(asio::ip::tcp::socket socket, std::weak_ptr<RtmpSessionObserver> obs);
    ~RtmpSession();

    void start();

    void set_group(std::weak_ptr<Group> group);

    void dispose() {}

  public:
    void async_send(BufferPtr buf);

  private:
    void close();

  private:
    void do_read_c0c1();
    void read_c0c1_cb(ErrorCode ec, std::size_t len);
    void do_write_s0s1();
    void write_s0s1_cb(ErrorCode ec, std::size_t len);
    void do_read_c2();
    void do_write_s2();
    void write_s2_cb(ErrorCode ec, std::size_t len);
    void read_c2_cb(ErrorCode ec, std::size_t len);
    void do_read();
    void read_cb(ErrorCode ec, std::size_t len);
    void do_write_win_ack_size();
    void write_win_ack_size_cb(ErrorCode ec, std::size_t len);
    void do_write_peer_bandwidth();
    void write_peer_bandwidth_cb(ErrorCode ec, std::size_t len);
    void do_write_chunk_size();
    void write_chunk_size_cb(ErrorCode ec, std::size_t len);
    void do_write_connect_result();
    void write_connect_result_cb(ErrorCode ec, std::size_t len);
    void do_write_create_stream_result();
    void write_create_stream_result_cb(ErrorCode ec, std::size_t len);

  private:
    void do_write_on_status_publish();
    void write_on_status_publish_cb(ErrorCode ec, std::size_t len);

  private:
    void do_write_on_status_play();
    void write_on_status_play_cb(ErrorCode ec, std::size_t len);

  private:
    void complete_message_handler();

    void protocol_control_message_handler();
    void set_chunk_size_handler(int val);
    void win_ack_size_handler(int val);

    void command_message_handler();
    void connect_handler();
    void release_stream_handler();
    void fcpublish_handler();
    void create_stream_handler();
    void publish_handler();
    void fcsubscribe_handler();
    void play_handler();

    void user_control_message_handler();

    void data_message_handler();

  private:
    void av_handler();

  private:
    void do_send();
    void send_cb(const ErrorCode &ec, std::size_t len);

  private:
    RtmpSession(const RtmpSession &) = delete;
    RtmpSession &operator=(const RtmpSession &) = delete;

  public:
    CHEF_PROPERTY_WITH_INIT_VALUE(bool, has_sent_avc_header, false);
    CHEF_PROPERTY_WITH_INIT_VALUE(bool, has_sent_key_frame, false);

  private:
    asio::ip::tcp::socket              socket_;
    std::weak_ptr<RtmpSessionObserver> obs_;
    RtmpHandshake                      rtmp_handshake_;
    chef::buffer                       read_buf_;
    chef::buffer                       write_buf_;
    BufferPtr                          complete_read_buf_;
    std::weak_ptr<Group>               group_;
    int                                timestamp_;
    int                                timestamp_base_ = -1;
    int                                msg_len_;
    int                                msg_type_id_;
    int                                msg_stream_id_;
    bool                               header_done_ = false;
    int                                peer_chunk_size_ = RTMP_DEFAULT_CHUNK_SIZE;
    int                                peer_win_ack_size_ = -1;
    double                             create_stream_transaction_id_ = -1;
    std::string                        app_;
    std::string                        live_name_;
    std::queue<BufferPtr>              send_buffers_;
    RtmpSessionType                    type_ = RTMP_SESSION_TYPE_UNKNOWN;
};

}
