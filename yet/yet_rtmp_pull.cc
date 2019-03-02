#include "yet_rtmp_pull.h"
#include "chef_base/chef_strings_op.hpp"
#include "yet_common/yet_log.h"
#include "yet_rtmp/yet_rtmp_pack_op.h"
#include "yet_rtmp/yet_rtmp_helper_op.h"

#define SNIPPET_ENTER_CB \
  do { \
    if (!ec) { \
    } else { \
      YET_LOG_ERROR("[{}] {} ec:{}", (void *)this, __func__, ec.message()); \
      if (ec == asio::error::eof) { \
        YET_LOG_INFO("[{}] close by peer.", (void *)this); \
        close(); \
      } else if (ec == asio::error::broken_pipe) { \
        YET_LOG_ERROR("[{}] broken pipe.", (void *)this); \
      } \
      return; \
    } \
  } while(0);

namespace yet {

RtmpPull::RtmpPull(asio::io_context &io_context)
  : RtmpSessionBase(io_context, RtmpSessionBaseType::RTMP_SESSION_TYPE_PULL)
  , resolver_(io_context)
  , write_buf_(BUF_INIT_LEN_RTMP_WRITE, BUF_SHRINK_LEN_RTMP_WRITE)
{
  YET_LOG_INFO("[{}] new rtmp pull.", (void *)this);
}

RtmpPull::~RtmpPull() {
  YET_LOG_INFO("[{}] delete rtmp pull.", (void *)this);
}

void RtmpPull::async_pull(const std::string url) {
  RtmpUrlStuff rus;
  // CHEFTODO check
  RtmpHelperOp::resolve_rtmp_url(url, rus);
  peer_ip_ = rus.host;
  peer_port_ = rus.port;
  do_tcp_connect();
}

void RtmpPull::async_pull(const std::string &peer_ip, uint16_t peer_port, const std::string &app_name, const std::string &live_name) {
  peer_ip_   = peer_ip;
  peer_port_ = peer_port;
  app_name_  = app_name;
  live_name_ = live_name;
  do_tcp_connect();
}

void RtmpPull::do_tcp_connect() {
  auto self(get_self());
  resolver_.async_resolve(peer_ip_, chef::strings_op::to_string(peer_port_),
                          [this, self](const ErrorCode &ec, asio::ip::tcp::resolver::results_type endpoints) {
                            SNIPPET_ENTER_CB;
                            asio::async_connect(socket_, endpoints,  std::bind(&RtmpPull::tcp_connect_cb, get_self(), _1, _2));
                          });
}

void RtmpPull::tcp_connect_cb(const ErrorCode &ec, const asio::ip::tcp::endpoint &) {
  SNIPPET_ENTER_CB;
  do_write_c0c1();
}

void RtmpPull::do_write_c0c1() {
  YET_LOG_INFO("[{}] ---->Handshake C0+C1", (void *)this);
  auto self(get_self());
  asio::async_write(socket_, asio::buffer(handshake_.create_c0c1(), RTMP_C0C1_LEN),
                    [this, self](const ErrorCode &ec, std::size_t) {
                      SNIPPET_ENTER_CB;
                      do_read_s0s1s2();
                    });
}

void RtmpPull::do_read_s0s1s2() {
  read_buf_.reserve(RTMP_S0S1S2_LEN);
  auto self(get_self());
  asio::async_read(socket_,
                   asio::buffer(read_buf_.write_pos(), RTMP_S0S1S2_LEN),
                   [this, self](const ErrorCode &ec, std::size_t) {
                      SNIPPET_ENTER_CB;
                      handshake_.handle_s0s1s2(read_buf_.read_pos());
                      YET_LOG_INFO("[{}] <----Handshake S0+S1+S2", (void *)this);
                      do_write_c2();
                   });
}

void RtmpPull::do_write_c2() {
  YET_LOG_INFO("[{}] ---->Handshake C2", (void *)this);
  auto self(get_self());
  asio::async_write(socket_, asio::buffer(handshake_.create_c2(), RTMP_C2_LEN),
                    [this, self](const ErrorCode &ec, std::size_t) {
                      SNIPPET_ENTER_CB;
                      do_write_chunk_size();
                    });
}

void RtmpPull::do_write_chunk_size() {
  auto wlen = RtmpPackOp::encode_rtmp_msg_chunk_size_reserve();
  write_buf_.reserve(wlen);
  RtmpPackOp::encode_chunk_size(write_buf_.write_pos(), RTMP_LOCAL_CHUNK_SIZE);
  YET_LOG_INFO("[{}] ----->Set Chunk Size {}", (void *)this, RTMP_LOCAL_CHUNK_SIZE);
  auto self(get_self());
  asio::async_write(socket_, asio::buffer(write_buf_.read_pos(), wlen),
                    [this, self](const ErrorCode &ec, std::size_t) {
                      SNIPPET_ENTER_CB;
                      do_write_rtmp_connect();
                    });
}

void RtmpPull::do_write_rtmp_connect() {
  std::ostringstream oss;
  oss << "rtmp://" << peer_ip_ << ":" << peer_port_ << "/" << app_name_ << "/" << live_name_;
  auto url = oss.str();
  auto wlen = RtmpPackOp::encode_rtmp_msg_connect_reserve(app_name_.c_str(), url.c_str(), url.c_str());
  write_buf_.reserve(wlen);
  RtmpPackOp::encode_connect(write_buf_.write_pos(), wlen, app_name_.c_str(), url.c_str(), url.c_str(), RTMP_TRANSACTION_ID_PUSH_PULL_CONNECT);
  YET_LOG_INFO("[{}] ----->connect(\'{}\')", (void *)this, app_name_);
  auto self(get_self());
  asio::async_write(socket_, asio::buffer(write_buf_.read_pos(), wlen),
                    [this, self](const ErrorCode &ec, std::size_t) {
                      SNIPPET_ENTER_CB;
                      do_read();
                    });
}

void RtmpPull::rtmp_connect_result_handler() {
  int wlen = RtmpPackOp::encode_rtmp_msg_create_stream_reserve();
  write_buf_.reserve(wlen);
  RtmpPackOp::encode_create_stream(write_buf_.write_pos(), RTMP_TRANSACTION_ID_PUSH_PULL_CREATE_STREAM);
  YET_LOG_INFO("[{}] ----->createStream()", (void *)this);
  auto self(get_self());
  asio::async_write(socket_, asio::buffer(write_buf_.read_pos(), wlen),
                    [this, self](const ErrorCode &ec, std::size_t) {
                      SNIPPET_ENTER_CB;
                    });
}

void RtmpPull::create_stream_result_handler() {
  int wlen = RtmpPackOp::encode_rtmp_msg_play_reserve(live_name_.c_str());
  write_buf_.reserve(wlen);
  RtmpPackOp::encode_play(write_buf_.write_pos(), wlen, live_name_.c_str(), stream_id_, RTMP_TRANSACTION_ID_PUSH_PULL_PLAY);
  YET_LOG_INFO("[{}] ----->play(\'{}\')", (void *)this, live_name_.c_str());
  auto self(get_self());
  asio::async_write(socket_, asio::buffer(write_buf_.read_pos(), wlen),
                    [this, self](const ErrorCode &ec, std::size_t) {
                      SNIPPET_ENTER_CB;
                    });
}

void RtmpPull::play_result_handler() {

}

RtmpPullPtr RtmpPull::get_self() {
  return std::dynamic_pointer_cast<RtmpPull>(shared_from_this());
}

}
