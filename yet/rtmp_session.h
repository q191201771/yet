/**
 * @file   rtmp_server.h
 * @author pengrl
 *
 */

#pragma once

#include <memory>
#include <asio.hpp>
#include <chef_base/chef_buffer.hpp>

namespace yet {

class RTMPSession : public std::enable_shared_from_this<RTMPSession> {
  public:
    RTMPSession(asio::ip::tcp::socket socket);
    ~RTMPSession();

    void start();

  private:
    void do_read_c0c1();
    void read_c0c1_cb(asio::error_code ec, std::size_t len);
    void do_write_s0s1();
    void write_s0s1_cb(asio::error_code ec, std::size_t len);
    void do_read_c2();
    void read_c2_cb(asio::error_code ec, std::size_t len);
    void do_write_s2();
    void write_s2_cb(asio::error_code ec, std::size_t len);
    void do_read();
    void read_cb(asio::error_code ec, std::size_t len);
    void do_write_win_ack_size();
    void write_win_ack_size_cb(asio::error_code ec, std::size_t len);
    void do_write_peer_bandwidth();
    void write_peer_bandwidth_cb(asio::error_code ec, std::size_t len);
    void do_write_chunk_size();
    void write_chunk_size_cb(asio::error_code ec, std::size_t len);
    void do_write_connect_result();
    void write_connect_result_cb(asio::error_code ec, std::size_t len);
    void do_write_create_stream_result();
    void write_create_stream_result_cb(asio::error_code ec, std::size_t len);
    void do_write_on_status_publish();
    void write_on_status_cb(asio::error_code ec, std::size_t len);

  private:
    void complete_message_handler();

    void protocol_control_message_handler();
    void set_chunk_size_handler();

    void command_message_handler();
    void connect_handler();
    void release_stream_handler();
    void fc_publish_handler();
    void create_stream_handler();
    void publish_handler();

    void data_message_handler();

    void audio_handler();
    void video_handler();

  private:
    RTMPSession(const RTMPSession &) = delete;
    RTMPSession &operator=(const RTMPSession &) = delete;

  private:
    asio::ip::tcp::socket socket_;
    chef::buffer read_buf_;
    chef::buffer write_buf_;
    chef::buffer complete_read_buf_;
    int timestamp_;
    int timestamp_base_ = -1;
    int msg_len_;
    int msg_type_id_;
    int msg_stream_id_;
    bool header_done_ = false;
    int peer_chunk_size_ = -1;
};

}
