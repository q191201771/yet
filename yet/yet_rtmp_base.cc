#include "yet_rtmp_base.h"
#include "yet_rtmp/yet_rtmp.hpp"
#include "yet_rtmp/yet_rtmp_amf_op.h"

namespace yet {

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

RtmpSessionBase::RtmpSessionBase(asio::io_context &io_context, RtmpSessionBaseType type)
  : socket_(io_context)
  , type_(type)
  , read_buf_(BUF_INIT_LEN_RTMP_EACH_READ, BUF_SHRINK_LEN_RTMP_EACH_READ)
{

}

RtmpSessionBase::RtmpSessionBase(asio::ip::tcp::socket socket)
  : socket_(std::move(socket))
  , read_buf_(BUF_INIT_LEN_RTMP_EACH_READ, BUF_SHRINK_LEN_RTMP_EACH_READ)
{
}

void RtmpSessionBase::do_read() {
  read_buf_.reserve(BUF_INIT_LEN_RTMP_EACH_READ);
  auto self(shared_from_this());
  socket_.async_read_some(asio::buffer(read_buf_.write_pos(), BUF_INIT_LEN_RTMP_EACH_READ),
                          [this, self](const ErrorCode &ec, std::size_t len) {
                            SNIPPET_ENTER_CB;
                            read_buf_.seek_write_pos(len);
                            rtmp_protocol_.try_compose(read_buf_, std::bind(&RtmpSessionBase::complete_message_handler, shared_from_this(), _1));
                            do_read();
                          });
}

void RtmpSessionBase::complete_message_handler(RtmpStreamPtr stream) {
  curr_stream_ = stream;

  switch (curr_stream_->header.msg_type_id) {
  case RTMP_MSG_TYPE_ID_WIN_ACK_SIZE: // S2C
  case RTMP_MSG_TYPE_ID_BANDWIDTH: // S2C
  case RTMP_MSG_TYPE_ID_SET_CHUNK_SIZE: // S2C
    protocol_control_message_handler();
    break;
  case RTMP_MSG_TYPE_ID_COMMAND_MESSAGE_AMF0: // S2C C2S
    command_message_handler();
    break;
  case RTMP_MSG_TYPE_ID_AUDIO: // S2C C2S
  case RTMP_MSG_TYPE_ID_VIDEO: // S2C C2S
    av_handler();
    break;
  default:
    YET_LOG_ASSERT(0, "unknown msg type. {}", curr_stream_->header.msg_type_id);
  }
}

void RtmpSessionBase::protocol_control_message_handler() {
  int val;
  AmfOp::decode_int32(curr_stream_->msg->read_pos(), 4, &val, nullptr);

  switch (curr_stream_->header.msg_type_id) {
  case RTMP_MSG_TYPE_ID_WIN_ACK_SIZE:
    ctrl_msg_win_ack_size_handler(val);
    break;
  case RTMP_MSG_TYPE_ID_SET_CHUNK_SIZE:
    ctrl_msg_set_chunk_size_handler(val);
    break;
  case RTMP_MSG_TYPE_ID_BANDWIDTH:
    YET_LOG_INFO("[{}] recvd protocol control message bandwidth, ignore it. bandwidth:{}", (void *)this, val);
    break;
  }
}

void RtmpSessionBase::ctrl_msg_set_chunk_size_handler(int val) {
  //peer_chunk_size_ = val;
  YET_LOG_INFO("[{}] ---->Set Chunk Size {}", (void *)this, val);
  rtmp_protocol_.update_peer_chunk_size(val);
}

void RtmpSessionBase::ctrl_msg_win_ack_size_handler(int val) {
  YET_LOG_INFO("[{}] ---->Window Acknowledgement Size {}", (void *)this, val);
  peer_win_ack_size_ = val;
}

void RtmpSessionBase::command_message_handler() {
  auto p = curr_stream_->msg->read_pos();
  std::string cmd;
  p = AmfOp::decode_string_with_type(p, curr_stream_->msg->readable_size(), &cmd, nullptr);

  double transaction_id;
  p = AmfOp::decode_number_with_type(p, curr_stream_->msg->write_pos()-p, &transaction_id, nullptr);

  auto left_size = curr_stream_->msg->write_pos()-p;
  if (cmd == "_result") {
    cmd_msg_result_handler(p, left_size, transaction_id);
  } else if (cmd == "onStatus") {
    cmd_msg_on_status_handler(p, left_size, transaction_id);
  } else {
    YET_LOG_ASSERT(0, "Unknown command:{}", cmd);
  }
}

void RtmpSessionBase::cmd_msg_result_handler(uint8_t *pos, std::size_t len, uint32_t tid) {
  if (type_ == RtmpSessionBaseType::RTMP_SESSION_TYPE_PULL) {
    if (tid == RTMP_TRANSACTION_ID_PUSH_PULL_CONNECT) {
      AmfObjectItemMap props;
      std::size_t used;
      pos = AmfOp::decode_object(pos, len, &props, &used);
      YET_LOG_ASSERT(pos, "decode object failed.");
      AmfObjectItemMap infos;
      pos = AmfOp::decode_object(pos, len-used, &infos, nullptr);
      YET_LOG_ASSERT(pos, "decode object failed.");
      auto code = infos.get("code");
      YET_LOG_ASSERT(code && code->is_string(), "invalid code filed in cmd msg.");
      if (code->get_string() == "NetConnection.Connect.Success") {
        YET_LOG_INFO("[{}] ----->_result(\'NetConnection.Connect.Success\')", (void *)this);
        rtmp_connect_result_handler();
      } else {
        YET_LOG_ASSERT(0, "invalid code {}", code->get_string());
      }
    } else if (tid == RTMP_TRANSACTION_ID_PUSH_PULL_CREATE_STREAM) {
      YET_LOG_ASSERT(*pos == Amf0DataType_NULL, "invalid.");
      pos++; len--;
      double dsid;
      pos = AmfOp::decode_number_with_type(pos, len, &dsid, nullptr);
      YET_LOG_ASSERT(pos, "invalid.");
      stream_id_ = dsid;
      YET_LOG_INFO("[{}] ----->_result()", (void *)this);
      create_stream_result_handler();
    } else {
      YET_LOG_ASSERT(0, "invalid tid. {}", tid);
    }
  }
}

void RtmpSessionBase::cmd_msg_on_status_handler(uint8_t *pos, std::size_t len, uint32_t tid) {
  if (type_ == RtmpSessionBaseType::RTMP_SESSION_TYPE_PULL) {
    YET_LOG_ASSERT(tid == 0, "invalid tid. {}", tid);
    YET_LOG_ASSERT(*pos == Amf0DataType_NULL, "invalid.");
    pos++; len--;
    AmfObjectItemMap infos;
    pos = AmfOp::decode_object(pos, len, &infos, nullptr);
    YET_LOG_ASSERT(pos, "decode object failed.");
    auto code = infos.get("code");
    YET_LOG_ASSERT(code && code->is_string(), "invalid code filed in cmd msg.");
    if (code->get_string() == "NetStream.Play.Start") {
      YET_LOG_INFO("[{}] ----->onStatus(\'NetStream.Play.Start\')", (void *)this);
      play_result_handler();
    } else {
      YET_LOG_ASSERT(0, "invalid code {}", code->get_string());
    }
  }
}

void RtmpSessionBase::av_handler() {
 // CHEFTODO
}

}
