#include "yet_rtmp_session_pub_sub.h"
#include <asio.hpp>
#include "chef_base/chef_stuff_op.hpp"
#include "yet.hpp"
#include "yet_rtmp/yet_rtmp.hpp"
#include "yet_rtmp/yet_rtmp_amf_op.h"
#include "yet_rtmp/yet_rtmp_pack_op.h"

namespace yet {

RtmpSessionPubSub::RtmpSessionPubSub(asio::ip::tcp::socket socket)
  : RtmpSessionBase(std::move(socket), RtmpSessionType::PUB_SUB)
  , write_buf_(BUF_INIT_LEN_RTMP_WRITE, BUF_SHRINK_LEN_RTMP_WRITE)
{
  YET_LOG_INFO("[{}] [lifecycle] new RtmpSessionPubSub.", (void *)this);
}

RtmpSessionPubSub::~RtmpSessionPubSub() {
  YET_LOG_DEBUG("[{}] [lifecycle] delete RtmpSessionPubSub.", (void *)this);
}

void RtmpSessionPubSub::start() {
  do_read_c0c1();
}

void RtmpSessionPubSub::do_read_c0c1() {
  read_buf_.reserve(RTMP_C0C1_LEN);
  auto self(get_self());
  asio::async_read(socket_,
                   asio::buffer(read_buf_.write_pos(), RTMP_C0C1_LEN),
                   [this, self](const ErrorCode &ec, std::size_t len) {
                     SNIPPET_RTMP_SESSION_ENTER_CB;
                     // CHEFTODO check ret val
                     handshake_.handle_c0c1(read_buf_.read_pos(), len);
                     YET_LOG_INFO("[{}] ---->Handshake C0+C1", (void *)this);
                     do_write_s0s1();
                   });
}

void RtmpSessionPubSub::do_write_s0s1() {
  YET_LOG_INFO("[{}] <----Handshake S0+S1", (void *)this);
  auto self(get_self());
  asio::async_write(socket_, asio::buffer(handshake_.create_s0s1(), RTMP_S0S1_LEN),
                    [this, self](const ErrorCode &ec, std::size_t) {
                      SNIPPET_RTMP_SESSION_ENTER_CB;
                      do_write_s2();
                    });
}

void RtmpSessionPubSub::do_write_s2() {
  YET_LOG_INFO("[{}] <----Handshake S2", (void *)this);
  auto self(get_self());
  asio::async_write(socket_, asio::buffer(handshake_.create_s2(), RTMP_S2_LEN),
                    [this, self](const ErrorCode &ec, std::size_t) {
                      SNIPPET_RTMP_SESSION_ENTER_CB;
                      do_read_c2();
                    });
}

void RtmpSessionPubSub::do_read_c2() {
  read_buf_.reserve(RTMP_C2_LEN);
  auto self(get_self());
  asio::async_read(socket_,
                   asio::buffer(read_buf_.write_pos(), RTMP_C2_LEN),
                   [this, self](const ErrorCode &ec, std::size_t) {
                     SNIPPET_RTMP_SESSION_ENTER_CB;
                     YET_LOG_INFO("[{}] ---->Handshake C2", (void *)this);
                     // CHEFTODO check c2
                     do_read();
                   });
}

void RtmpSessionPubSub::on_command_message(const std::string &cmd, uint32_t tid, uint8_t *pos, std::size_t len) {
  if (cmd == "releaseStream" ||
      cmd == "FCPublish" ||
      cmd == "FCUnpublish" ||
      cmd == "FCSubscribe" ||
      cmd == "getStreamLength"
  ) {
    YET_LOG_INFO("[{}] recvd command message {},ignore it.", (void *)this, cmd);

  } else if (cmd == "connect")      { connect_handler(tid, pos, len);
  } else if (cmd == "createStream") { create_stream_handler(tid, pos, len);
  } else if (cmd == "publish")      { publish_handler(tid, pos, len);
  } else if (cmd == "play")         { play_handler(tid, pos, len);
  } else if (cmd == "deleteStream") { delete_stream_handler(tid, pos, len);
  } else {
    YET_LOG_ASSERT(0, "Unknown command:{}", cmd);
  }
}

void RtmpSessionPubSub::connect_handler(double transaction_id, uint8_t *buf, std::size_t len) {
  curr_tid_ = transaction_id;
  //YET_LOG_ASSERT(transaction_id == RTMP_TRANSACTION_ID_CONNECT, "invalid transaction_id while rtmp connect. {}", transaction_id)

  AmfObjectItemMap objs;
  buf = AmfOp::decode_object(buf, len, &objs, nullptr);
  YET_LOG_ASSERT(buf, "decode command connect failed.");

  AmfObjectItem *app = objs.get("app");
  YET_LOG_ASSERT(app && app->is_string(), "invalid app field when rtmp connect.");
  app_name_ = app->get_string();

  YET_LOG_INFO("[{}] ---->connect(\'{}\')", (void *)this, app_name_);

  // type
  // obs nonprivate

  AmfObjectItem *type = objs.get("type");
  AmfObjectItem *flash_ver = objs.get("flashVer");
  AmfObjectItem *swf_url = objs.get("swfUrl");
  AmfObjectItem *tc_url = objs.get("tcUrl");
  YET_LOG_INFO("[{}] connect app:{}, type:{}, flashVer:{}, swfUrl:{}, tcUrl:{}",
               (void *)this, app_name_,
               type ? type->stringify() : "",
               flash_ver ? flash_ver->stringify() : "",
               swf_url ? swf_url->stringify() : "",
               tc_url ? tc_url->stringify() : ""
              );

  do_write_win_ack_size();
}

void RtmpSessionPubSub::do_write_win_ack_size() {
  auto wlen = RtmpPackOp::encode_win_ack_size_reserve();
  write_buf_.reserve(wlen);
  RtmpPackOp::encode_win_ack_size(write_buf_.write_pos(), RTMP_WINDOW_ACKNOWLEDGEMENT_SIZE);
  YET_LOG_INFO("[{}] <----Window Acknowledgement Size {}", (void *)this, RTMP_WINDOW_ACKNOWLEDGEMENT_SIZE);
  auto self(get_self());
  asio::async_write(socket_, asio::buffer(write_buf_.read_pos(), wlen),
                    [this, self](const ErrorCode &ec, std::size_t) {
                      SNIPPET_RTMP_SESSION_ENTER_CB;
                      do_write_peer_bandwidth();
                    });
}

void RtmpSessionPubSub::do_write_peer_bandwidth() {
  auto wlen = RtmpPackOp::encode_peer_bandwidth_reserve();
  write_buf_.reserve(wlen);
  RtmpPackOp::encode_peer_bandwidth(write_buf_.write_pos(), RTMP_PEER_BANDWIDTH);
  YET_LOG_INFO("[{}] <----Set Peer Bandwidth {},Dynamic", (void *)this, RTMP_PEER_BANDWIDTH);
  auto self(get_self());
  asio::async_write(socket_, asio::buffer(write_buf_.read_pos(), wlen),
                    [this, self](const ErrorCode &ec, std::size_t) {
                      SNIPPET_RTMP_SESSION_ENTER_CB;
                      do_write_chunk_size();
                    });
}

void RtmpSessionPubSub::do_write_chunk_size() {
  auto wlen = RtmpPackOp::encode_chunk_size_reserve();
  write_buf_.reserve(wlen);
  RtmpPackOp::encode_chunk_size(write_buf_.write_pos(), RTMP_LOCAL_CHUNK_SIZE);
  YET_LOG_INFO("[{}] <----Set Chunk Size {}", (void *)this, RTMP_LOCAL_CHUNK_SIZE);
  auto self(get_self());
  asio::async_write(socket_, asio::buffer(write_buf_.read_pos(), wlen),
                    [this, self](const ErrorCode &ec, std::size_t) {
                      SNIPPET_RTMP_SESSION_ENTER_CB;
                      do_write_connect_result();
                    });
}

void RtmpSessionPubSub::do_write_connect_result() {
  auto wlen = RtmpPackOp::encode_connect_result_reserve();
  write_buf_.reserve(wlen);
  RtmpPackOp::encode_connect_result(write_buf_.write_pos(), curr_tid_);
  YET_LOG_INFO("[{}] <----_result(\'NetConnection.Connect.Success\')", (void *)this);
  auto self(get_self());
  asio::async_write(socket_, asio::buffer(write_buf_.read_pos(), wlen),
                    [this, self](const ErrorCode &ec, std::size_t) {
                      SNIPPET_RTMP_SESSION_ENTER_CB;
                    });
}

void RtmpSessionPubSub::create_stream_handler(double transaction_id, uint8_t *buf, std::size_t len) {
  (void)buf; (void)len;
  curr_tid_ = transaction_id;

  // TODO null obj

  YET_LOG_INFO("[{}] ---->createStream()", (void *)this);
  do_write_create_stream_result();
}

void RtmpSessionPubSub::do_write_create_stream_result() {
  auto wlen = RtmpPackOp::encode_create_stream_result_reserve();
  write_buf_.reserve(wlen);
  // TODO stream id
  RtmpPackOp::encode_create_stream_result(write_buf_.write_pos(), curr_tid_);
  YET_LOG_INFO("[{}] <----_result()", (void *)this);
  auto self(get_self());
  asio::async_write(socket_, asio::buffer(write_buf_.read_pos(), wlen),
                    [this, self](const ErrorCode &ec, std::size_t) {
                      SNIPPET_RTMP_SESSION_ENTER_CB;
                    });
}

void RtmpSessionPubSub::publish_handler(double transaction_id, uint8_t *buf, std::size_t len) {
  (void)transaction_id;
  std::size_t used_len;
  //YET_LOG_ASSERT(transaction_id == RTMP_TRANSACTION_ID_PUBLISH, "invalid transaction_id while rtmp publish. {}", transaction_id)
  buf++; // skip null
  len--;
  buf = AmfOp::decode_string_with_type(buf, len, &stream_name_, &used_len);
  len -= used_len;
  YET_LOG_ASSERT(buf, "invalid publish name field when rtmp publish.");
  std::string pub_type;
  buf = AmfOp::decode_string_with_type(buf, len, &pub_type, &used_len);
  len -= used_len;
  YET_LOG_ASSERT(buf, "invalid publish name field when rtmp publish.");

  YET_LOG_INFO("[{}] ---->publish(\'{}\')", (void *)this, stream_name_);
  type_ = RtmpSessionType::PUB;
  if (pub_start_cb_) {
    pub_start_cb_(get_self());
  }

  do_write_on_status_publish();
}

void RtmpSessionPubSub::do_write_on_status_publish() {
  auto wlen = RtmpPackOp::encode_on_status_publish_reserve();
  write_buf_.reserve(wlen);
  // TODO stream id
  RtmpPackOp::encode_on_status_publish(write_buf_.write_pos(), 1);
  YET_LOG_INFO("[{}] <----onStatus(\'NetStream.Publish.Start\')", (void *)this);
  auto self(get_self());
  asio::async_write(socket_, asio::buffer(write_buf_.read_pos(), wlen),
                    [this, self](const ErrorCode &ec, std::size_t) {
                      SNIPPET_RTMP_SESSION_ENTER_CB;
                    });
}

void RtmpSessionPubSub::play_handler(double transaction_id, uint8_t *buf, std::size_t len) {
  (void)transaction_id;
  //YET_LOG_ASSERT(transaction_id == RTMP_TRANSACTION_ID_PLAY, "invalid transaction id while rtmp play. {}", transaction_id);
  buf++; // skip null
  len--;
  buf = AmfOp::decode_string_with_type(buf, len, &stream_name_, nullptr);
  YET_LOG_INFO("[{}] ---->play(\'{}\')", (void *)this, stream_name_);

  // TODO
  // start duration reset

  do_write_on_status_play();
}

void RtmpSessionPubSub::do_write_on_status_play() {
  auto wlen = RtmpPackOp::encode_on_status_play_reserve();
  write_buf_.reserve(wlen);
  // TODO stream id
  RtmpPackOp::encode_on_status_play(write_buf_.write_pos(), 1);
  YET_LOG_INFO("[{}] <----onStatus(\'NetStream.Play.Start\')", (void *)this);
  auto self(get_self());
  asio::async_write(socket_, asio::buffer(write_buf_.read_pos(), wlen),
                    [this, self](const ErrorCode &ec, std::size_t) {
                      SNIPPET_RTMP_SESSION_ENTER_CB;
                      type_ = RtmpSessionType::SUB;
                      if (sub_start_cb_) {
                        sub_start_cb_(get_self());
                      }
                    });
}

void RtmpSessionPubSub::delete_stream_handler(double transaction_id, uint8_t *buf, std::size_t len) {
  (void)transaction_id;
  buf++;
  len--;
  double msid;
  AmfOp::decode_number_with_type(buf, len, &msid, nullptr);
  YET_LOG_INFO("[{}] ----->deleteStream({})", (void *)this, msid);
  if (type_ == RtmpSessionType::PUB) {
    if (pub_stop_cb_) {
      pub_stop_cb_(get_self());
    }
  }
}

void RtmpSessionPubSub::set_pub_start_cb(tmpRtmpEventCb cb) {
  pub_start_cb_ = cb;
}

void RtmpSessionPubSub::set_pub_stop_cb(tmpRtmpEventCb cb) {
  pub_stop_cb_ = cb;
}

void RtmpSessionPubSub::set_sub_start_cb(tmpRtmpEventCb cb) {
  sub_start_cb_ = cb;
}

RtmpSessionPubSubPtr RtmpSessionPubSub::get_self() {
  return std::dynamic_pointer_cast<RtmpSessionPubSub>(shared_from_this());
}

} // namespace yet
