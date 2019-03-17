#include "yet_rtmp_session_push_pull.h"
#include "chef_base/chef_strings_op.hpp"
#include "yet_common/yet_log.h"
#include "yet_rtmp/yet_rtmp_pack_op.h"
#include "yet_rtmp/yet_rtmp_helper_op.h"
#include "yet_rtmp/yet_rtmp_amf_op.h"

namespace yet {

RtmpSessionPushPull::RtmpSessionPushPull(asio::io_context &io_ctx, RtmpSessionType type)
  : RtmpSessionBase(io_ctx, type)
  , resolver_(io_ctx)
  , write_buf_(BUF_INIT_LEN_RTMP_WRITE, BUF_SHRINK_LEN_RTMP_WRITE)
{
  YET_LOG_INFO("[{}] [lifecycle] new RtmpSessionPushPull.", (void *)this);
}

RtmpSessionPushPull::~RtmpSessionPushPull() {
  YET_LOG_INFO("[{}] [lifecycle] delete RtmpSessionPushPull.", (void *)this);
}

std::shared_ptr<RtmpSessionPushPull> RtmpSessionPushPull::create_push(asio::io_context &io_ctx) {
  return std::shared_ptr<RtmpSessionPushPull>(new RtmpSessionPushPull(io_ctx, RtmpSessionType::PUSH));
}

std::shared_ptr<RtmpSessionPushPull> RtmpSessionPushPull::create_pull(asio::io_context &io_ctx) {
  return std::shared_ptr<RtmpSessionPushPull>(new RtmpSessionPushPull(io_ctx, RtmpSessionType::PULL));
}

RtmpSessionPushPullPtr RtmpSessionPushPull::get_self() {
  return std::dynamic_pointer_cast<RtmpSessionPushPull>(shared_from_this());
}

void RtmpSessionPushPull::async_start(const std::string url) {
  RtmpUrlStuff rus;
  auto ret = RtmpHelperOp::resolve_rtmp_url(url, rus);
  SINPPET_RTMP_SESSION_ASSERT(ret, "invalid url. {}", url);
  async_start(rus.host, rus.port, rus.app_name, rus.stream_name);
}

void RtmpSessionPushPull::async_start(const std::string &peer_ip, uint16_t peer_port, const std::string &app_name, const std::string &stream_name) {
  peer_ip_     = peer_ip;
  peer_port_   = peer_port;
  app_name_    = app_name;
  stream_name_ = stream_name;
  do_tcp_connect();
}

void RtmpSessionPushPull::do_tcp_connect() {
  auto self(get_self());
  resolver_.async_resolve(peer_ip_, chef::strings_op::to_string(peer_port_),
                          [this, self](const ErrorCode &ec, asio::ip::tcp::resolver::results_type endpoints) {
                            SNIPPET_RTMP_SESSION_ENTER_CB;
                            asio::async_connect(socket_, endpoints,  std::bind(&RtmpSessionPushPull::tcp_connect_handler, get_self(), _1, _2));
                          });
}

void RtmpSessionPushPull::tcp_connect_handler(const ErrorCode &ec, const asio::ip::tcp::endpoint &) {
  SNIPPET_RTMP_SESSION_ENTER_CB;
  do_write_c0c1();
}

void RtmpSessionPushPull::do_write_c0c1() {
  YET_LOG_INFO("[{}] <----Handshake C0+C1", (void *)this);
  auto self(get_self());
  asio::async_write(socket_, asio::buffer(handshake_.create_c0c1(), RTMP_C0C1_LEN),
                    [this, self](const ErrorCode &ec, size_t) {
                      SNIPPET_RTMP_SESSION_ENTER_CB;
                      do_read_s0s1s2();
                    });
}

void RtmpSessionPushPull::do_read_s0s1s2() {
  read_buf_.reserve(RTMP_S0S1S2_LEN);
  auto self(get_self());
  asio::async_read(socket_,
                   asio::buffer(read_buf_.write_pos(), RTMP_S0S1S2_LEN),
                   [this, self](const ErrorCode &ec, size_t) {
                      SNIPPET_RTMP_SESSION_ENTER_CB;
                      handshake_.handle_s0s1s2(read_buf_.read_pos());
                      YET_LOG_INFO("[{}] ---->Handshake S0+S1+S2", (void *)this);
                      do_write_c2();
                   });
}

void RtmpSessionPushPull::do_write_c2() {
  YET_LOG_INFO("[{}] <----Handshake C2", (void *)this);
  auto self(get_self());
  asio::async_write(socket_, asio::buffer(handshake_.create_c2(), RTMP_C2_LEN),
                    [this, self](const ErrorCode &ec, size_t) {
                      SNIPPET_RTMP_SESSION_ENTER_CB;
                      do_write_chunk_size();
                    });
}

void RtmpSessionPushPull::do_write_chunk_size() {
  auto wlen = RtmpPackOp::encode_chunk_size_reserve();
  write_buf_.reserve(wlen);
  RtmpPackOp::encode_chunk_size(write_buf_.write_pos(), RTMP_LOCAL_CHUNK_SIZE);
  YET_LOG_INFO("[{}] <-----Set Chunk Size {}", (void *)this, RTMP_LOCAL_CHUNK_SIZE);
  auto self(get_self());
  asio::async_write(socket_, asio::buffer(write_buf_.read_pos(), wlen),
                    [this, self](const ErrorCode &ec, size_t) {
                      SNIPPET_RTMP_SESSION_ENTER_CB;
                      do_write_rtmp_connect();
                    });
}

void RtmpSessionPushPull::do_write_rtmp_connect() {
  std::ostringstream oss;
  oss << "rtmp://" << peer_ip_ << ":" << peer_port_ << "/" << app_name_ << "/" << stream_name_;
  auto url = oss.str();
  auto wlen = RtmpPackOp::encode_connect_reserve(app_name_.c_str(), url.c_str(), url.c_str());
  write_buf_.reserve(wlen);
  RtmpPackOp::encode_connect(write_buf_.write_pos(), wlen, app_name_.c_str(), url.c_str(), url.c_str(), RTMP_TRANSACTION_ID_PUSH_PULL_CONNECT);
  YET_LOG_INFO("[{}] <-----connect(\'{}\')", (void *)this, app_name_);
  auto self(get_self());
  asio::async_write(socket_, asio::buffer(write_buf_.read_pos(), wlen),
                    [this, self](const ErrorCode &ec, size_t) {
                      SNIPPET_RTMP_SESSION_ENTER_CB;
                      do_read();
                    });
}

void RtmpSessionPushPull::on_command_message(const std::string &cmd, uint32_t tid, uint8_t *pos, size_t len) {
  if (cmd == "onBWDone") {
    YET_LOG_INFO("[{}] recvd command message {},ignore it.", (void *)this, cmd);

  } else if (cmd == "_result") { cmd_msg_result_handler(pos, len, tid);
  } else if (cmd == "onStatus") { cmd_msg_on_status_handler(pos, len, tid);
  } else {
    SINPPET_RTMP_SESSION_ASSERT(0, "invalid command message. {}", cmd);
  }
}

void RtmpSessionPushPull::cmd_msg_result_handler(uint8_t *pos, size_t len, uint32_t tid) {
  if (tid == RTMP_TRANSACTION_ID_PUSH_PULL_CONNECT) {
    AmfObjectItemMap props;
    size_t used;
    pos = AmfOp::decode_object(pos, len, &props, &used);
    SINPPET_RTMP_SESSION_ASSERT(pos, "invalid _result message.");

    AmfObjectItemMap infos;
    pos = AmfOp::decode_object(pos, len-used, &infos, nullptr);
    SINPPET_RTMP_SESSION_ASSERT(pos, "invalid _result message.");

    // CHEFTOO other code
    auto code = infos.get("code");
    SINPPET_RTMP_SESSION_ASSERT(code && code->is_string() && code->get_string() == "NetConnection.Connect.Success",
                                "invalid _result message.");
    YET_LOG_INFO("[{}] ----->_result(\'NetConnection.Connect.Success\')", (void *)this);
    do_write_create_stream();
  }

  if (tid == RTMP_TRANSACTION_ID_PUSH_PULL_CREATE_STREAM) {
    SNIPPET_RTMP_SESSION_SKIP_AMF_NULL(pos, len);

    double dsid;
    pos = AmfOp::decode_number_with_type(pos, len, &dsid, nullptr);
    SINPPET_RTMP_SESSION_ASSERT(pos, "invalid _result message.");
    stream_id_ = dsid;

    YET_LOG_INFO("[{}] ----->_result()", (void *)this);
    if (type_ == RtmpSessionType::PUSH) {
      do_write_publish();
    } else if (type_ == RtmpSessionType::PULL) {
      do_write_play();
    }
  }

}

void RtmpSessionPushPull::cmd_msg_on_status_handler(uint8_t *pos, size_t len, uint32_t tid) {
  SINPPET_RTMP_SESSION_ASSERT(tid == 0, "invalid onStatus message.");
  SNIPPET_RTMP_SESSION_SKIP_AMF_NULL(pos, len);

  AmfObjectItemMap infos;
  pos = AmfOp::decode_object(pos, len, &infos, nullptr);
  SINPPET_RTMP_SESSION_ASSERT(pos, "invalid onStatus message.");
  auto code = infos.get("code");
  SINPPET_RTMP_SESSION_ASSERT(code && code->is_string(), "invalid onStatus message.");

  if (type_ == RtmpSessionType::PUSH) {
    if (code->get_string() == "NetStream.Publish.Start") {
      YET_LOG_INFO("[{}] ----->onStatus(\'NetStream.Publish.Start\')", (void *)this);
      is_ready_ = true;
      return;
    }
  } else if (type_ == RtmpSessionType::PULL) {
    if (code->get_string() == "NetStream.Play.Start") {
      YET_LOG_INFO("[{}] ----->onStatus(\'NetStream.Play.Start\')", (void *)this);
      return;
    }
  }
  SINPPET_RTMP_SESSION_ASSERT(0, "invalid onStatus message. {}", code->get_string());
}

void RtmpSessionPushPull::do_write_create_stream() {
  auto wlen = RtmpPackOp::encode_create_stream_reserve();
  write_buf_.reserve(wlen);
  RtmpPackOp::encode_create_stream(write_buf_.write_pos(), RTMP_TRANSACTION_ID_PUSH_PULL_CREATE_STREAM);
  YET_LOG_INFO("[{}] <-----createStream()", (void *)this);
  auto self(get_self());
  asio::async_write(socket_, asio::buffer(write_buf_.read_pos(), wlen),
                    [this, self](const ErrorCode &ec, size_t) {
                      SNIPPET_RTMP_SESSION_ENTER_CB;
                    });
}

void RtmpSessionPushPull::do_write_play() {
  auto wlen = RtmpPackOp::encode_play_reserve(stream_name_.c_str());
  write_buf_.reserve(wlen);
  RtmpPackOp::encode_play(write_buf_.write_pos(), wlen, stream_name_.c_str(), stream_id_, RTMP_TRANSACTION_ID_PUSH_PULL_PLAY);
  YET_LOG_INFO("[{}] <-----play(\'{}\')", (void *)this, stream_name_.c_str());
  auto self(get_self());
  asio::async_write(socket_, asio::buffer(write_buf_.read_pos(), wlen),
                    [this, self](const ErrorCode &ec, size_t) {
                      SNIPPET_RTMP_SESSION_ENTER_CB;
                    });
}

void RtmpSessionPushPull::do_write_publish() {
  auto wlen = RtmpPackOp::encode_publish_reserve(app_name_.c_str(), stream_name_.c_str());
  write_buf_.reserve(wlen);
  RtmpPackOp::encode_publish(write_buf_.write_pos(), wlen, app_name_.c_str(), stream_name_.c_str(), stream_id_, RTMP_TRANSACTION_ID_PUSH_PULL_PUBLISH);
  YET_LOG_INFO("[{}] <-----publish(\'{}\')", (void *)this, stream_name_.c_str());
  auto self(get_self());
  asio::async_write(socket_, asio::buffer(write_buf_.read_pos(), wlen),
                    [this, self](const ErrorCode &ec, size_t) {
                      SNIPPET_RTMP_SESSION_ENTER_CB;
                    });
}

}
