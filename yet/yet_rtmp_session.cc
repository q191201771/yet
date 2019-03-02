#include "yet_rtmp_session.h"
#include <asio.hpp>
#include "yet.hpp"
#include "yet_rtmp/yet_rtmp.hpp"
#include "yet_rtmp/yet_rtmp_amf_op.h"
#include "yet_rtmp/yet_rtmp_pack_op.h"
#include "chef_base/chef_stuff_op.hpp"

namespace yet {

//YET_LOG_DEBUG("{} len:{}", __func__, len);
#define SNIPPET_ENTER_CB \
  do { \
    if (!ec) { \
    } else { \
      YET_LOG_ERROR("[{}] {} ec:{}, len:{}", (void *)this, __func__, ec.message(), len); \
      if (ec == asio::error::eof) { \
        YET_LOG_INFO("[{}] close by peer.", (void *)this); \
        close(); \
      } else if (ec == asio::error::broken_pipe) { \
        YET_LOG_ERROR("[{}] broken pipe.", (void *)this); \
      } \
      return; \
    } \
  } while(0);


//#define SNIPPET_KEEP_READ do { do_read(); return; } while(0);

//#define SNIPPET_ASYNC_READ(pos, len, func)      asio::async_read(socket_, asio::buffer(pos, len), std::bind(func, shared_from_this(), _1, _2));
#define SNIPPET_ASYNC_READ_SOME(pos, len, func) socket_.async_read_some(asio::buffer(pos, len), std::bind(func, shared_from_this(), _1, _2));
//#define SNIPPET_ASYNC_WRITE(pos, len, func)     asio::async_write(socket_, asio::buffer(pos, len), std::bind(func, shared_from_this(), _1, _2));

RtmpSession::RtmpSession(asio::ip::tcp::socket socket)
  : socket_(std::move(socket))
  , read_buf_(std::max(BUF_INIT_LEN_RTMP_EACH_READ, RTMP_C0C1_LEN), BUF_SHRINK_LEN_RTMP_EACH_READ)
  , write_buf_(BUF_INIT_LEN_RTMP_WRITE, BUF_SHRINK_LEN_RTMP_WRITE)
{
  YET_LOG_INFO("[{}] new rtmp session.", (void *)this);
}

RtmpSession::~RtmpSession() {
  YET_LOG_DEBUG("[{}] delete rtmp session.", (void *)this);
}

void RtmpSession::start() {
  do_read_c0c1();
}

void RtmpSession::do_read_c0c1() {
  read_buf_.reserve(RTMP_C0C1_LEN);
  auto self(shared_from_this());
  asio::async_read(socket_,
                   asio::buffer(read_buf_.write_pos(), RTMP_C0C1_LEN),
                   [this, self](const ErrorCode &ec, std::size_t len) {
                     SNIPPET_ENTER_CB;
                     // CHEFTODO check ret val
                     handshake_.handle_c0c1(read_buf_.read_pos(), len);
                     YET_LOG_INFO("[{}] ---->Handshake C0+C1", (void *)this);
                     do_write_s0s1();
                   });
}

void RtmpSession::do_write_s0s1() {
  YET_LOG_INFO("[{}] <----Handshake S0+S1", (void *)this);
  auto self(shared_from_this());
  asio::async_write(socket_,
                    asio::buffer(handshake_.create_s0s1(), RTMP_S0S1_LEN),
                    [this, self](const ErrorCode &ec, std::size_t len) {
                      SNIPPET_ENTER_CB;
                      do_write_s2();
                    });
}

void RtmpSession::do_write_s2() {
  YET_LOG_INFO("[{}] <----Handshake S2", (void *)this);
  auto self(shared_from_this());
  asio::async_write(socket_,
                    asio::buffer(handshake_.create_s2(), RTMP_S2_LEN),
                    [this, self](const ErrorCode &ec, std::size_t len) {
                      SNIPPET_ENTER_CB;
                      do_read_c2();
                    });
}

void RtmpSession::do_read_c2() {
  read_buf_.reserve(RTMP_C2_LEN);
  auto self(shared_from_this());
  asio::async_read(socket_,
                   asio::buffer(read_buf_.write_pos(), RTMP_C2_LEN),
                   [this, self](const ErrorCode &ec, std::size_t len) {
                     SNIPPET_ENTER_CB;
                     YET_LOG_INFO("[{}] ---->Handshake C2", (void *)this);
                     // CHEFTODO check c2
                     do_read();
                   });
}

void RtmpSession::do_read() {
  read_buf_.reserve(BUF_INIT_LEN_RTMP_EACH_READ);
  SNIPPET_ASYNC_READ_SOME(read_buf_.write_pos(), BUF_INIT_LEN_RTMP_EACH_READ, &RtmpSession::read_cb);
}

void RtmpSession::read_cb(const ErrorCode &ec, std::size_t len) {
  SNIPPET_ENTER_CB;

  read_buf_.seek_write_pos(len);
  //YET_LOG_DEBUG("[{}] > read_cb. read len:{}, readable_size:{}", (void *)this, len, read_buf_.readable_size());

  rtmp_protocol_.try_compose(read_buf_, std::bind(&RtmpSession::complete_message_handler, shared_from_this(), _1));
  do_read();
}

void RtmpSession::complete_message_handler(RtmpStreamPtr stream) {
  //YET_LOG_DEBUG("[{}] > complete message handler. type:{}", (void *)this, curr_stream_->header.msg_type_id);
  curr_stream_ = stream;

  switch (curr_stream_->header.msg_type_id) {
  case RTMP_MSG_TYPE_ID_SET_CHUNK_SIZE:
  case RTMP_MSG_TYPE_ID_ABORT:
  case RTMP_MSG_TYPE_ID_ACK:
  case RTMP_MSG_TYPE_ID_WIN_ACK_SIZE:
  case RTMP_MSG_TYPE_ID_BANDWIDTH:
    protocol_control_message_handler();
    break;
  case RTMP_MSG_TYPE_ID_COMMAND_MESSAGE_AMF0:
    command_message_handler();
    break;
  case RTMP_MSG_TYPE_ID_DATA_MESSAGE_AMF0:
    data_message_handler();
    break;
  case RTMP_MSG_TYPE_ID_USER_CONTROL:
    user_control_message_handler();
    break;
  case RTMP_MSG_TYPE_ID_AUDIO:
  case RTMP_MSG_TYPE_ID_VIDEO:
    av_handler();
    break;
  default:
    YET_LOG_ASSERT(0, "unknown msg type. {}", curr_stream_->header.msg_type_id);
  }
}

void RtmpSession::protocol_control_message_handler() {
  int val;
  AmfOp::decode_int32(curr_stream_->msg->read_pos(), 4, &val, nullptr);

  switch (curr_stream_->header.msg_type_id) {
  case RTMP_MSG_TYPE_ID_SET_CHUNK_SIZE:
    set_chunk_size_handler(val);
    break;
  case RTMP_MSG_TYPE_ID_WIN_ACK_SIZE:
    win_ack_size_handler(val);
    break;
  case RTMP_MSG_TYPE_ID_ABORT:
    YET_LOG_INFO("[{}] recvd protocol control message abort, ignore it. csid:{}", (void *)this, val);
    break;
  case RTMP_MSG_TYPE_ID_ACK:
    YET_LOG_INFO("[{}] recvd protocol control message ack, ignore it. seq num:{}", (void *)this, val);
    break;
  case RTMP_MSG_TYPE_ID_BANDWIDTH:
    YET_LOG_INFO("[{}] recvd protocol control message bandwidth, ignore it. bandwidth:{}", (void *)this, val);
    break;
  default:
    YET_LOG_ASSERT(0, "unknown protocol control message. msg type id:{}", curr_stream_->header.msg_type_id);
  }
}

void RtmpSession::set_chunk_size_handler(int val) {
  //peer_chunk_size_ = val;
  YET_LOG_INFO("[{}] ---->Set Chunk Size {}", (void *)this, val);
  rtmp_protocol_.update_peer_chunk_size(val);
}

void RtmpSession::win_ack_size_handler(int val) {
  YET_LOG_INFO("[{}] ---->Window Acknowledgement Size {}", (void *)this, val);
  peer_win_ack_size_ = val;
}

void RtmpSession::command_message_handler() {
  uint8_t *p = curr_stream_->msg->read_pos();
  std::string cmd;
  p = AmfOp::decode_string_with_type(p, curr_stream_->msg->readable_size(), &cmd, nullptr);

  double transaction_id;
  p = AmfOp::decode_number_with_type(p, curr_stream_->msg->write_pos()-p, &transaction_id, nullptr);

  std::size_t left_size = curr_stream_->msg->write_pos()-p;
  if (cmd == "releaseStream" ||
      cmd == "FCPublish" ||
      cmd == "FCUnpublish" ||
      cmd == "FCSubscribe" ||
      cmd == "getStreamLength"
  ) {
    YET_LOG_INFO("[{}] recvd command message {},ignore it.", (void *)this, cmd);

  } else if (cmd == "connect")      { connect_handler(transaction_id, p, left_size);
  } else if (cmd == "createStream") { create_stream_handler(transaction_id, p, left_size);
  } else if (cmd == "publish")      { publish_handler(transaction_id, p, left_size);
  } else if (cmd == "play")         { play_handler(transaction_id, p, left_size);
  } else if (cmd == "deleteStream") { delete_stream_handler(transaction_id, p, left_size);
  } else {
    YET_LOG_ASSERT(0, "Unknown command:{}", cmd);
  }
}

void RtmpSession::connect_handler(double transaction_id, uint8_t *buf, std::size_t len) {
  curr_tid_ = transaction_id;
  //YET_LOG_ASSERT(transaction_id == RTMP_TRANSACTION_ID_CONNECT, "invalid transaction_id while rtmp connect. {}", transaction_id)

  AmfObjectItemMap objs;
  buf = AmfOp::decode_object(buf, len, &objs, nullptr);
  YET_LOG_ASSERT(buf, "decode command connect failed.");

  AmfObjectItem *app = objs.get("app");
  YET_LOG_ASSERT(app && app->is_string(), "invalid app field when rtmp connect.");
  app_ = app->get_string();

  YET_LOG_INFO("[{}] ---->connect(\'{}\')", (void *)this, app_);

  // type
  // obs nonprivate

  AmfObjectItem *type = objs.get("type");
  AmfObjectItem *flash_ver = objs.get("flashVer");
  AmfObjectItem *swf_url = objs.get("swfUrl");
  AmfObjectItem *tc_url = objs.get("tcUrl");
  YET_LOG_INFO("[{}] connect app:{}, type:{}, flashVer:{}, swfUrl:{}, tcUrl:{}",
               (void *)this, app_,
               type ? type->stringify() : "",
               flash_ver ? flash_ver->stringify() : "",
               swf_url ? swf_url->stringify() : "",
               tc_url ? tc_url->stringify() : ""
              );

  do_write_win_ack_size();
}

void RtmpSession::do_write_win_ack_size() {
  int wlen = RtmpPackOp::encode_rtmp_msg_win_ack_size_reserve();
  write_buf_.reserve(wlen);
  RtmpPackOp::encode_win_ack_size(write_buf_.write_pos(), RTMP_WINDOW_ACKNOWLEDGEMENT_SIZE);
  YET_LOG_INFO("[{}] <----Window Acknowledgement Size {}", (void *)this, RTMP_WINDOW_ACKNOWLEDGEMENT_SIZE);
  auto self(shared_from_this());
  asio::async_write(socket_,
                    asio::buffer(write_buf_.read_pos(), wlen),
                    [this, self](const ErrorCode &ec, std::size_t len) {
                      SNIPPET_ENTER_CB;
                      do_write_peer_bandwidth();
                    });
}

void RtmpSession::do_write_peer_bandwidth() {
  int wlen = RtmpPackOp::encode_rtmp_msg_peer_bandwidth_reserve();
  write_buf_.reserve(wlen);
  RtmpPackOp::encode_peer_bandwidth(write_buf_.write_pos(), RTMP_PEER_BANDWIDTH);
  YET_LOG_INFO("[{}] <----Set Peer Bandwidth {},Dynamic", (void *)this, RTMP_PEER_BANDWIDTH);
  auto self(shared_from_this());
  asio::async_write(socket_,
                    asio::buffer(write_buf_.read_pos(), wlen),
                    [this, self](const ErrorCode &ec, std::size_t len) {
                      SNIPPET_ENTER_CB;
                      do_write_chunk_size();
                    });
}

void RtmpSession::do_write_chunk_size() {
  int wlen = RtmpPackOp::encode_rtmp_msg_chunk_size_reserve();
  write_buf_.reserve(wlen);
  RtmpPackOp::encode_chunk_size(write_buf_.write_pos(), RTMP_LOCAL_CHUNK_SIZE);
  YET_LOG_INFO("[{}] <----Set Chunk Size {}", (void *)this, RTMP_LOCAL_CHUNK_SIZE);
  auto self(shared_from_this());
  asio::async_write(socket_,
                    asio::buffer(write_buf_.read_pos(), wlen),
                    [this, self](const ErrorCode &ec, std::size_t len) {
                      SNIPPET_ENTER_CB;
                      do_write_connect_result();
                    });
}

void RtmpSession::do_write_connect_result() {
  int wlen = RtmpPackOp::encode_rtmp_msg_connect_result_reserve();
  write_buf_.reserve(wlen);
  RtmpPackOp::encode_connect_result(write_buf_.write_pos(), curr_tid_);
  YET_LOG_INFO("[{}] <----_result(\'NetConnection.Connect.Success\')", (void *)this);
  auto self(shared_from_this());
  asio::async_write(socket_,
                    asio::buffer(write_buf_.read_pos(), wlen),
                    [this, self](const ErrorCode &ec, std::size_t len) {
                      SNIPPET_ENTER_CB;
                    });
}

void RtmpSession::create_stream_handler(double transaction_id, uint8_t *buf, std::size_t len) {
  (void)buf; (void)len;
  curr_tid_ = transaction_id;

  // TODO null obj

  YET_LOG_INFO("[{}] ---->createStream()", (void *)this);
  do_write_create_stream_result();
}

void RtmpSession::do_write_create_stream_result() {
  int wlen = RtmpPackOp::encode_rtmp_msg_create_stream_result_reserve();
  write_buf_.reserve(wlen);
  // TODO stream id
  RtmpPackOp::encode_create_stream_result(write_buf_.write_pos(), curr_tid_);
  YET_LOG_INFO("[{}] <----_result()", (void *)this);
  auto self(shared_from_this());
  asio::async_write(socket_,
                    asio::buffer(write_buf_.read_pos(), wlen),
                    [this, self](const ErrorCode &ec, std::size_t len) {
                      SNIPPET_ENTER_CB;
                    });
}

void RtmpSession::publish_handler(double transaction_id, uint8_t *buf, std::size_t len) {
  (void)transaction_id;
  std::size_t used_len;
  //YET_LOG_ASSERT(transaction_id == RTMP_TRANSACTION_ID_PUBLISH, "invalid transaction_id while rtmp publish. {}", transaction_id)
  buf++; // skip null
  len--;
  buf = AmfOp::decode_string_with_type(buf, len, &live_name_, &used_len);
  len -= used_len;
  YET_LOG_ASSERT(buf, "invalid publish name field when rtmp publish.");
  std::string pub_type;
  buf = AmfOp::decode_string_with_type(buf, len, &pub_type, &used_len);
  len -= used_len;
  YET_LOG_ASSERT(buf, "invalid publish name field when rtmp publish.");

  YET_LOG_INFO("[{}] ---->publish(\'{}\')", (void *)this, live_name_);
  type_ = RTMP_SESSION_TYPE_PUB;
  if (rtmp_publish_cb_) {
    rtmp_publish_cb_(shared_from_this());
  }

  do_write_on_status_publish();
}

void RtmpSession::do_write_on_status_publish() {
  int wlen = RtmpPackOp::encode_rtmp_msg_on_status_publish_reserve();
  write_buf_.reserve(wlen);
  // TODO stream id
  RtmpPackOp::encode_on_status_publish(write_buf_.write_pos(), 1);
  YET_LOG_INFO("[{}] <----onStatus(\'NetStream.Publish.Start\')", (void *)this);
  auto self(shared_from_this());
  asio::async_write(socket_,
                    asio::buffer(write_buf_.read_pos(), wlen),
                    [this, self](const ErrorCode &ec, std::size_t len) {
                      SNIPPET_ENTER_CB;
                    });
}

void RtmpSession::play_handler(double transaction_id, uint8_t *buf, std::size_t len) {
  (void)transaction_id;
  //YET_LOG_ASSERT(transaction_id == RTMP_TRANSACTION_ID_PLAY, "invalid transaction id while rtmp play. {}", transaction_id);
  buf++; // skip null
  len--;
  buf = AmfOp::decode_string_with_type(buf, len, &live_name_, nullptr);
  YET_LOG_INFO("[{}] ---->play(\'{}\')", (void *)this, live_name_);

  // TODO
  // start duration reset

  do_write_on_status_play();
}

void RtmpSession::do_write_on_status_play() {
  int wlen = RtmpPackOp::encode_rtmp_msg_on_status_play_reserve();
  write_buf_.reserve(wlen);
  // TODO stream id
  RtmpPackOp::encode_on_status_play(write_buf_.write_pos(), 1);
  YET_LOG_INFO("[{}] <----onStatus(\'NetStream.Play.Start\')", (void *)this);
  auto self(shared_from_this());
  asio::async_write(socket_,
                    asio::buffer(write_buf_.read_pos(), wlen),
                    [this, self](const ErrorCode &ec, std::size_t len) {
                      SNIPPET_ENTER_CB;
                      type_ = RTMP_SESSION_TYPE_SUB;
                      if (rtmp_play_cb_) {
                        rtmp_play_cb_(shared_from_this());
                      }
                    });
}

void RtmpSession::delete_stream_handler(double transaction_id, uint8_t *buf, std::size_t len) {
  (void)transaction_id;
  buf++;
  len--;
  double msid;
  AmfOp::decode_number_with_type(buf, len, &msid, nullptr);
  YET_LOG_INFO("[{}] ----->deleteStream({})", (void *)this, msid);
  if (type_ == RTMP_SESSION_TYPE_PUB) {
    if (rtmp_publish_stop_cb_) {
      rtmp_publish_stop_cb_(shared_from_this());
    }
  }
}

void RtmpSession::user_control_message_handler() {
  YET_LOG_ERROR("[{}] TODO", (void *)this);
}

void RtmpSession::data_message_handler() {
  // 7.1.2.

  uint8_t *p = curr_stream_->msg->read_pos();
  auto len = curr_stream_->msg->readable_size();

  std::string val;
  std::size_t used_len;
  p = AmfOp::decode_string_with_type(p, len, &val, &used_len);
  YET_LOG_ASSERT(p, "decode metadata failed.");
  if (val != "@setDataFrame") {
    YET_LOG_ERROR("invalid data message. {}", val);
    return;
  }
  len -= used_len;
  uint8_t *meta_pos = p;
  std::size_t meta_size = len;
  p = AmfOp::decode_string_with_type(p, len, &val, &used_len);
  YET_LOG_ASSERT(p, "decode metadata failed.");
  if (val != "onMetaData") {
    YET_LOG_ERROR("invalid data message. {}", val);
    return;
  }
  len -= used_len;
  std::shared_ptr<AmfObjectItemMap> metadata = std::make_shared<AmfObjectItemMap>();
  p = AmfOp::decode_ecma_array(p, len, metadata.get(), nullptr);
  YET_LOG_ASSERT(p, "decode metadata failed.");
  YET_LOG_DEBUG("ts:{}, type id:{}, {}", curr_stream_->timestamp_abs, curr_stream_->header.msg_type_id, metadata->stringify());
  if (rtmp_meta_data_cb_) {
    rtmp_meta_data_cb_(shared_from_this(), curr_stream_->msg, meta_pos, meta_size, metadata);
  }
}

void RtmpSession::av_handler() {
  //YET_LOG_DEBUG("[{}] -----recvd {} {}. ts:{} {}, size:{}", (void *this), curr_csid_, curr_stream_->header.msg_type_id, curr_stream_->header.timestamp, curr_stream_->timestamp_abs, curr_stream_->msg->readable_size());

  RtmpHeader h;
  h.csid = curr_stream_->header.msg_type_id == RTMP_MSG_TYPE_ID_AUDIO ? RTMP_CSID_AUDIO : RTMP_CSID_VIDEO;
  h.timestamp = curr_stream_->timestamp_abs;
  h.msg_len = curr_stream_->msg->readable_size();
  h.msg_type_id = curr_stream_->header.msg_type_id;
  h.msg_stream_id = RTMP_MSID;
  if (rtmp_av_data_cb_) {
    rtmp_av_data_cb_(shared_from_this(), curr_stream_->msg, h);
  }
}

void RtmpSession::async_send(BufferPtr buf) {
  bool is_empty = send_buffers_.empty();
  send_buffers_.push(buf);
  if (is_empty) {
    do_send();
  }
}

void RtmpSession::do_send() {
  BufferPtr buf = send_buffers_.front();
  asio::async_write(socket_, asio::buffer(buf->read_pos(), buf->readable_size()),
                    std::bind(&RtmpSession::send_cb, shared_from_this(), _1, _2));
}

void RtmpSession::send_cb(const ErrorCode &ec, std::size_t len) {
  SNIPPET_ENTER_CB;

  send_buffers_.pop();
  if (!send_buffers_.empty()) {
    do_send();
  }
}

void RtmpSession::close() {
  YET_LOG_DEBUG("[{}] close rtmp session.", (void *)this);
  socket_.close();
  if (rtmp_session_close_cb_) {
    rtmp_session_close_cb_(shared_from_this());
  }
}

//RtmpStreamPtr RtmpSession::get_or_create_stream(int csid) {
//  auto &stream = csid2stream_[csid];
//  if (!stream) {
//    YET_LOG_DEBUG("[{}] create chunk stream. {}", (void *)this, csid);
//    stream = std::make_shared<RtmpStream>();
//    stream->msg = std::make_shared<Buffer>(BUF_INIT_LEN_RTMP_COMPLETE_MESSAGE, BUF_SHRINK_LEN_RTMP_COMPLETE_MESSAGE);
//  }
//  return stream;
//}

void RtmpSession::set_rtmp_publish_cb(RtmpEventCb cb) {
  rtmp_publish_cb_ = cb;
}

void RtmpSession::set_rtmp_play_cb(RtmpEventCb cb) {
  rtmp_play_cb_ = cb;
}

void RtmpSession::set_rtmp_publish_stop_cb(RtmpEventCb cb) {
  rtmp_publish_stop_cb_ = cb;
}

void RtmpSession::set_rtmp_session_close_cb(RtmpEventCb cb) {
  rtmp_session_close_cb_ = cb;
}

void RtmpSession::set_rtmp_meta_data_cb(RtmpMetaDataCb cb) {
  rtmp_meta_data_cb_ = cb;
}

void RtmpSession::set_rtmp_av_data_cb(RtmpAvDataCb cb) {
  rtmp_av_data_cb_ = cb;
}

} // namespace yet
