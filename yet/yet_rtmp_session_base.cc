#include "yet_rtmp_session_base.h"
#include "chef_base/chef_stuff_op.hpp"
#include "yet_rtmp/yet_rtmp.hpp"
#include "yet_rtmp/yet_rtmp_amf_op.h"
#include "yet_rtmp_session_server.h"
#include "yet_rtmp_session_client.h"

namespace yet {

RtmpSessionBase::RtmpSessionBase(asio::io_context &io_ctx, RtmpSessionType type)
  : socket_(io_ctx)
  , type_(type)
  , read_buf_(BUF_INIT_LEN_RTMP_EACH_READ, BUF_SHRINK_LEN_RTMP_EACH_READ)
{
}

RtmpSessionBase::RtmpSessionBase(asio::ip::tcp::socket socket, RtmpSessionType type)
  : socket_(std::move(socket))
  , type_(type)
  , read_buf_(BUF_INIT_LEN_RTMP_EACH_READ, BUF_SHRINK_LEN_RTMP_EACH_READ)
{
}

RtmpSessionType RtmpSessionBase::type() { return type_; }

void RtmpSessionBase::set_rtmp_session_close_cb(RtmpEventCb cb) { rtmp_session_close_cb_ = cb; }

void RtmpSessionBase::set_rtmp_meta_data_cb(RtmpMetaDataCb cb) { rtmp_meta_data_cb_ = cb; }

void RtmpSessionBase::set_rtmp_av_data_cb(RtmpAvDataCb cb) { rtmp_av_data_cb_ = cb; }

RtmpSessionServerPtr RtmpSessionBase::cast_to_server_session() {
  SINPPET_RTMP_SESSION_ASSERT(type_ == RtmpSessionType::SERVER || type_ == RtmpSessionType::PUB || type_ == RtmpSessionType::SUB,
                              "invalid type while cast base session to server session. {}", static_cast<int>(type_));
  return std::dynamic_pointer_cast<RtmpSessionServer>(shared_from_this());
}

RtmpSessionClientPtr RtmpSessionBase::cast_to_client_session() {
  SINPPET_RTMP_SESSION_ASSERT(type_ == RtmpSessionType::PUSH || type_ == RtmpSessionType::PULL,
                              "invalid type while cast base session to client session. {}", static_cast<int>(type_));
  return std::dynamic_pointer_cast<RtmpSessionClient>(shared_from_this());
}

void RtmpSessionBase::do_read() {
  read_buf_.reserve(BUF_INIT_LEN_RTMP_EACH_READ);
  auto self(shared_from_this());
  socket_.async_read_some(asio::buffer(read_buf_.write_pos(), BUF_INIT_LEN_RTMP_EACH_READ),
                          [this, self](const ErrorCode &ec, size_t len) {
                            SNIPPET_RTMP_SESSION_ENTER_CB;
                            read_buf_.seek_write_pos(len);
                            rtmp_protocol_.try_compose(read_buf_, [self](auto stream) { self->base_complete_message_handler(stream); });
                            do_read();
                          });
}

void RtmpSessionBase::base_complete_message_handler(RtmpStreamPtr stream) {
  curr_stream_ = stream;

  switch (curr_stream_->header.msg_type_id) {
  case RTMP_MSG_TYPE_ID_WIN_ACK_SIZE: // S2C
  case RTMP_MSG_TYPE_ID_BANDWIDTH: // S2C
  case RTMP_MSG_TYPE_ID_SET_CHUNK_SIZE: // S2C C2S
  case RTMP_MSG_TYPE_ID_ABORT:
  case RTMP_MSG_TYPE_ID_ACK:
    base_protocol_control_message_handler();
    break;
  case RTMP_MSG_TYPE_ID_COMMAND_MESSAGE_AMF0: // S2C C2S
    base_command_message_handler();
    break;
  case RTMP_MSG_TYPE_ID_USER_CONTROL:
    base_user_control_message_handler();
    break;
  case RTMP_MSG_TYPE_ID_DATA_MESSAGE_AMF0:
    base_meta_data_handler();
    break;
  case RTMP_MSG_TYPE_ID_AUDIO: // S2C C2S
  case RTMP_MSG_TYPE_ID_VIDEO: // S2C C2S
    base_av_data_handler();
    break;
  default:
    SINPPET_RTMP_SESSION_ASSERT(0, "invalid message type id. {}", curr_stream_->header.msg_type_id);
  }
}

void RtmpSessionBase::base_protocol_control_message_handler() {
  int val;
  uint8_t *p = AmfOp::decode_int32(curr_stream_->msg->read_pos(), curr_stream_->msg->readable_size(), &val, nullptr);
  SINPPET_RTMP_SESSION_ASSERT(p, "invalid protocol control message.");

  switch (curr_stream_->header.msg_type_id) {
  case RTMP_MSG_TYPE_ID_WIN_ACK_SIZE:
    base_ctrl_msg_win_ack_size_handler(val);
    break;
  case RTMP_MSG_TYPE_ID_SET_CHUNK_SIZE:
    base_ctrl_msg_set_chunk_size_handler(val);
    break;
  case RTMP_MSG_TYPE_ID_BANDWIDTH:
    YET_LOG_INFO("[{}] recvd protocol control message bandwidth, ignore it. bandwidth:{}", (void *)this, val);
    break;
  case RTMP_MSG_TYPE_ID_ABORT:
    YET_LOG_INFO("[{}] recvd protocol control message abort, ignore it. csid:{}", (void *)this, val);
    break;
  case RTMP_MSG_TYPE_ID_ACK:
    YET_LOG_INFO("[{}] recvd protocol control message ack, ignore it. seq num:{}", (void *)this, val);
    break;
  default:
    SINPPET_RTMP_SESSION_ASSERT(0, "invalid message type id. {}", curr_stream_->header.msg_type_id);
  }
}

void RtmpSessionBase::base_ctrl_msg_set_chunk_size_handler(int val) {
  // S2C C2S
  YET_LOG_INFO("[{}] ---->Set Chunk Size {}", (void *)this, val);
  rtmp_protocol_.update_peer_chunk_size(val);
}

void RtmpSessionBase::base_ctrl_msg_win_ack_size_handler(int val) {
  // S2C C2S
  YET_LOG_INFO("[{}] ---->Window Acknowledgement Size {}", (void *)this, val);
  peer_win_ack_size_ = val;
}

void RtmpSessionBase::base_command_message_handler() {
  auto p = curr_stream_->msg->read_pos();
  std::string cmd;
  p = AmfOp::decode_string_with_type(p, curr_stream_->msg->readable_size(), &cmd, nullptr);
  SINPPET_RTMP_SESSION_ASSERT(p, "invalid command message.");

  double tid;
  p = AmfOp::decode_number_with_type(p, curr_stream_->msg->write_pos()-p, &tid, nullptr);
  SINPPET_RTMP_SESSION_ASSERT(p, "invalid command message.");

  auto left_size = curr_stream_->msg->write_pos()-p;
  on_command_message(cmd, tid, p, left_size);
}

void RtmpSessionBase::base_user_control_message_handler() {
  // 7.1.7.
  YET_LOG_ERROR("[{}] TODO", (void *)this);
}

void RtmpSessionBase::close() {
  YET_LOG_DEBUG("[{}] close session.", (void *)this);
  socket_.close();
  if (rtmp_session_close_cb_) { rtmp_session_close_cb_(shared_from_this()); }
}

void RtmpSessionBase::base_meta_data_handler() {
  // 7.1.2.
  uint8_t *p = curr_stream_->msg->read_pos();
  auto len = curr_stream_->msg->readable_size();

  uint8_t *meta_pos = p;
  size_t meta_size = len;

  std::string val;
  size_t used_len;
  p = AmfOp::decode_string_with_type(p, len, &val, &used_len);
  SINPPET_RTMP_SESSION_ASSERT(p, "invalid meta data message.");
  len -= used_len;

  if (val == "@setDataFrame") {
    meta_pos = p;
    meta_size = len;

    p = AmfOp::decode_string_with_type(p, len, &val, &used_len);
    SINPPET_RTMP_SESSION_ASSERT(p, "invalid meta data message.");
    SINPPET_RTMP_SESSION_ASSERT(val == "onMetaData", "invalid meta data message. {}", val);
    len -= used_len;
  } else if (val == "onMetaData") {
    // do nothing
  } else {
    SINPPET_RTMP_SESSION_ASSERT(0, "invalid meta data message. {}", val);
  }

  std::shared_ptr<AmfObjectItemMap> metadata = std::make_shared<AmfObjectItemMap>();
  if (*p == Amf0DataType_ECMA_ARRAY) {
    p = AmfOp::decode_ecma_array(p, len, metadata.get(), nullptr);
    SINPPET_RTMP_SESSION_ASSERT(p, "invalid meta data message.");
  } else if (*p == Amf0DataType_OBJECT) {
    p = AmfOp::decode_object(p, len, metadata.get(), nullptr);
    SINPPET_RTMP_SESSION_ASSERT(p, "invalid meta data message.");
  } else {
    SINPPET_RTMP_SESSION_ASSERT(0, "invalid meta data message. {}", (int)*p);
  }

  if (rtmp_meta_data_cb_) {
    rtmp_meta_data_cb_(shared_from_this(), curr_stream_->msg, { meta_pos, meta_size }, metadata);
  }
}

void RtmpSessionBase::base_av_data_handler() {
  //YET_LOG_DEBUG("[{}] -----recvd {} {}. ts:{} {}, size:{}", (void *this), curr_csid_, curr_stream_->header.msg_type_id, curr_stream_->header.timestamp, curr_stream_->timestamp_abs, curr_stream_->msg->readable_size());

  RtmpHeader h;
  h.csid = curr_stream_->header.msg_type_id == RTMP_MSG_TYPE_ID_AUDIO ? RTMP_CSID_AUDIO : RTMP_CSID_VIDEO;
  h.timestamp = curr_stream_->timestamp_abs;
  h.msg_len = curr_stream_->msg->readable_size();
  h.msg_type_id = curr_stream_->header.msg_type_id;
  h.msg_stream_id = RTMP_MSID;
  if (rtmp_av_data_cb_) { rtmp_av_data_cb_(shared_from_this(), curr_stream_->msg, h); }
}

void RtmpSessionBase::async_send(BufferPtr buf) {
  SINPPET_RTMP_SESSION_ASSERT(type_ == RtmpSessionType::SUB || type_ == RtmpSessionType::PUSH, "invalid type while send data. {}", static_cast<int>(type_));
  auto is_empty = send_buffers_.empty();
  send_buffers_.push(buf);
  if (is_empty) { do_send(); }
}

void RtmpSessionBase::do_send() {
  auto buf = send_buffers_.front();
  auto self(shared_from_this());
  asio::async_write(socket_, asio::buffer(buf->read_pos(), buf->readable_size()),
                    [this, self](const ErrorCode &ec, size_t) {
                      SNIPPET_RTMP_SESSION_ENTER_CB;
                      send_buffers_.pop();
                      if (!send_buffers_.empty()) {
                        do_send();
                      }
                    });
}

}
