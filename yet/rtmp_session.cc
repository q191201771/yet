#include "rtmp_session.h"
#include "yet_inner.hpp"
#include "rtmp_amf_op.h"
#include "rtmp_pack_op.h"
#include <chef_base/chef_stuff_op.hpp>

namespace yet {

#define SNIPPET_ENTER_CB \
  do { \
    if (!ec) { \
      YET_LOG_DEBUG("{} len:{}", __func__, len); \
    } else { \
      YET_LOG_ERROR("{} ec:{}, len:{}", __func__, ec.message(), len); \
      return; \
    } \
  } while(0);

#define SNIPPET_KEEP_READ do { do_read(); return; } while(0);

#define SNIPPET_ASYNC_READ(pos, len, func)    asio::async_read(socket_, asio::buffer(pos, len), std::bind(func, shared_from_this(), _1, _2));
#define SNIPPET_ASYNC_READ_SOME(pos, len, func) socket_.async_read_some(asio::buffer(pos, len), std::bind(func, shared_from_this(), _1, _2));
#define SNIPPET_ASYNC_WRITE(pos, len, func)  asio::async_write(socket_, asio::buffer(pos, len), std::bind(func, shared_from_this(), _1, _2));

RTMPSession::RTMPSession(asio::ip::tcp::socket socket)
  : socket_(std::move(socket))
  , read_buf_(EACH_READ_LEN)
  , write_buf_(FIXED_WRITE_BUF_RESERVE_LEN)
  , complete_read_buf_(COMPLETE_MESSAGE_INIT_LEN, COMPLETE_MESSAGE_SHRINK_LEN)
{
  YET_LOG_INFO("RTMPSession() {}.", static_cast<void *>(this));
}

RTMPSession::~RTMPSession() {
  YET_LOG_INFO("~RTMPSession() {}.", static_cast<void *>(this));
}

void RTMPSession::start() {
  do_read_c0c1();
}

void RTMPSession::do_read_c0c1() {
  SNIPPET_ASYNC_READ(read_buf_.write_pos(), C0C1_LEN, &RTMPSession::read_c0c1_cb);
}

void RTMPSession::read_c0c1_cb(ErrorCode ec, std::size_t len) {
  SNIPPET_ENTER_CB;
  rtmp_handshake_.handle_c0c1(read_buf_.read_pos(), len);
  YET_LOG_INFO("---->Handshake C0+C1");
  do_write_s0s1();
}

void RTMPSession::do_write_s0s1() {
  YET_LOG_INFO("<----Handshake S0+S1");
  SNIPPET_ASYNC_WRITE(rtmp_handshake_.create_s0s1(), S0S1_LEN, &RTMPSession::write_s0s1_cb);
}

void RTMPSession::write_s0s1_cb(ErrorCode ec, std::size_t len) {
  SNIPPET_ENTER_CB;
  do_write_s2();
}

void RTMPSession::do_write_s2() {
  YET_LOG_INFO("<----Handshake S2");
  SNIPPET_ASYNC_WRITE(rtmp_handshake_.create_s2(), S2_LEN, &RTMPSession::write_s2_cb);
}

void RTMPSession::write_s2_cb(ErrorCode ec, std::size_t len) {
  SNIPPET_ENTER_CB;
  do_read_c2();
}

void RTMPSession::do_read_c2() {
  read_buf_.reserve(C2_LEN);
  SNIPPET_ASYNC_READ(read_buf_.write_pos(), C2_LEN, &RTMPSession::read_c2_cb);
}

void RTMPSession::read_c2_cb(ErrorCode ec, std::size_t len) {
  SNIPPET_ENTER_CB;
  YET_LOG_INFO("---->Handshake C2");
  do_read();
}

void RTMPSession::do_read() {
  read_buf_.reserve(EACH_READ_LEN);
  SNIPPET_ASYNC_READ_SOME(read_buf_.write_pos(), 209, &RTMPSession::read_cb);
}

void RTMPSession::read_cb(ErrorCode ec, std::size_t len) {
  SNIPPET_ENTER_CB;

  read_buf_.seek_write_pos(len);
  //YET_LOG_INFO("{}", chef::stuff_op::bytes_to_hex((const uint8_t *)read_buf_.read_pos(), read_buf_.readable_size(), 16));

  for (; read_buf_.readable_size() > 0; ) {
    YET_LOG_DEBUG("Enter message parse loop. read_buf readable_size:{}", read_buf_.readable_size());
    uint8_t *p = read_buf_.read_pos();

    int csid;
    if (!header_done_) {
      std::size_t readable_size = read_buf_.readable_size();

      // 5.3.1.1. Chunk Basic Header 1,2,3bytes
      int basic_header_len;
      uint8_t fmt;
      fmt = (*p >> 6) & 0x03;    // 2 bits
      csid = *p & 0x3F; // 6 bits
      if (csid == 0) {
        basic_header_len = 2;
        if (readable_size < 2) { SNIPPET_KEEP_READ; }

        csid = 64 + *(p+1);
      } else if (csid == 1) {
        basic_header_len = 3;
        if (readable_size < 3) { SNIPPET_KEEP_READ; }

        csid = 64 + *(p+1) + (*(p+2) * 256);
      } else if (csid == 2) {
        basic_header_len = 1;
      } else if (csid < 64) {
        basic_header_len = 1;
      } else {
        YET_LOG_ERROR("CHEFFUCKME. fmt:{},csid:{}", fmt, csid);
        return;
      }

      YET_LOG_DEBUG("Parsed basic header. {} fmt:{}, csid:{}, basic_header_len:{}", (unsigned char)*p, fmt, csid, basic_header_len);

      p += basic_header_len;
      readable_size -= basic_header_len;

      // 5.3.1.2. Chunk Message Header 11,7,3,0bytes
      if (fmt == 0) {
        if (readable_size < fmt_2_msg_header_len[fmt]) { SNIPPET_KEEP_READ; }

        p = AmfOp::decode_int24(p, 3, &timestamp_, nullptr);
        p = AmfOp::decode_int24(p, 3, &msg_len_, nullptr);
        msg_type_id_ = *p++;
        p = AmfOp::decode_int32_le(p, 4, &msg_stream_id_, nullptr);
      } else if (fmt == 1) {
        if (readable_size < fmt_2_msg_header_len[fmt]) { SNIPPET_KEEP_READ; }

        p = AmfOp::decode_int24(p, 3, &timestamp_, nullptr);
        p = AmfOp::decode_int24(p, 3, &msg_len_, nullptr);
        msg_type_id_ = *p++;
      } else if (fmt == 2) {
        if (readable_size < fmt_2_msg_header_len[fmt]) { SNIPPET_KEEP_READ; }

        p = AmfOp::decode_int24(p, 3, &timestamp_, nullptr);
      } else if (fmt == 3) {
        // noop
      } else {
        YET_LOG_ERROR("CHEFFUCKME.");
        return;
      }

      readable_size -= fmt_2_msg_header_len[fmt];

      // 5.3.1.3 Extended Timestamp
      bool has_ext_ts;
      has_ext_ts = timestamp_ == MAX_TIMESTAMP_IN_MSG_HEADER;
      if (has_ext_ts) {
        if (readable_size < 4) { SNIPPET_KEEP_READ; }

        p = AmfOp::decode_int32(p, 4, &timestamp_, nullptr);

        readable_size -= 4;
      }

      header_done_ = true;
      read_buf_.erase(basic_header_len + fmt_2_msg_header_len[fmt] + (has_ext_ts ? 4 : 0));

      YET_LOG_DEBUG("Parsed chunk message header. msg_header_len:{}, timestamp:{}, msg_len:{}, msg_type_id:{}, msg_stream_id:{}",
                    fmt_2_msg_header_len[fmt], timestamp_, msg_len_, msg_type_id_, msg_stream_id_);
    }

    int needed_size;
    if (msg_len_ <= peer_chunk_size_) {
      needed_size = msg_len_;
    } else {
      int whole_needed = msg_len_ - (int)complete_read_buf_.readable_size();
      needed_size = std::min(whole_needed, peer_chunk_size_);
    }

    if ((int)read_buf_.readable_size() < needed_size) { SNIPPET_KEEP_READ; }

    complete_read_buf_.append(read_buf_.read_pos(), needed_size);
    read_buf_.erase(needed_size);

    if ((int)complete_read_buf_.readable_size() == msg_len_) {
      complete_message_handler();
      complete_read_buf_.clear();
    }
    YET_LOG_ASSERT((int)complete_read_buf_.readable_size() <= msg_len_, "readable size invalid.");

    header_done_ = false;
  }

  do_read();
}

void RTMPSession::complete_message_handler() {
  switch (msg_type_id_) {
  case MSG_TYPE_ID_SET_CHUNK_SIZE:
  case MSG_TYPE_ID_ABORT:
  case MSG_TYPE_ID_ACK:
  case MSG_TYPE_ID_WIN_ACK_SIZE:
  case MSG_TYPE_ID_BANDWIDTH:
    protocol_control_message_handler();
    break;
  case MSG_TYPE_ID_COMMAND_MESSAGE_AMF0:
    command_message_handler();
    break;
  case MSG_TYPE_ID_DATA_MESSAGE_AMF0:
    data_message_handler();
    break;
  case MSG_TYPE_ID_USER_CONTROL:
    user_control_message_handler();
    break;
  case MSG_TYPE_ID_AUDIO:
    audio_handler();
    break;
  case MSG_TYPE_ID_VIDEO:
    video_handler();
    break;
  default:
    YET_LOG_ERROR("CHEFFUCKME. {}", msg_type_id_);
  }
}

void RTMPSession::audio_handler() {
  int timestamp_abs = (timestamp_base_ == -1) ? timestamp_ : (timestamp_base_ + timestamp_);
  YET_LOG_INFO("-----Recvd audio. ts:{}, delta:{}, size:{}", timestamp_abs, timestamp_, complete_read_buf_.readable_size());

  timestamp_base_ = timestamp_abs;
}

void RTMPSession::video_handler() {
  int timestamp_abs = (timestamp_base_ == -1) ? timestamp_ : (timestamp_base_ + timestamp_);
  YET_LOG_INFO("-----Recvd video. ts:{}, delta:{}, size:{}", timestamp_abs, timestamp_, complete_read_buf_.readable_size());

  timestamp_base_ = timestamp_abs;
}

void RTMPSession::data_message_handler() {
  YET_LOG_WARN("TODO");
}

void RTMPSession::user_control_message_handler() {
  YET_LOG_WARN("TODO");
}

void RTMPSession::protocol_control_message_handler() {
  int val;
  AmfOp::decode_int32(complete_read_buf_.read_pos(), 4, &val, nullptr);

  switch (msg_type_id_) {
  case MSG_TYPE_ID_SET_CHUNK_SIZE:
    set_chunk_size_handler(val);
    break;
  case MSG_TYPE_ID_ABORT:
    YET_LOG_INFO("Recv protocol control message abort, ignore it. csid:{}", val);
    break;
  case MSG_TYPE_ID_ACK:
    YET_LOG_INFO("Recv protocol control message ack, ignore it. seq num:{}", val);
    break;
  case MSG_TYPE_ID_WIN_ACK_SIZE:
    win_ack_size_handler(val);
    break;
  case MSG_TYPE_ID_BANDWIDTH:
    YET_LOG_INFO("Recv protocol control message bandwidth, ignore it. bandwidth:{}", val);
    break;
  default:
    YET_LOG_ASSERT(false, "Unknown protocol control message. msg type id:{}", msg_type_id_);
  }
}

void RTMPSession::set_chunk_size_handler(int val) {
  peer_chunk_size_ = val;
  YET_LOG_INFO("---->Set Chunk Size {}", peer_chunk_size_);
}

void RTMPSession::win_ack_size_handler(int val) {
  YET_LOG_INFO("---->Window Acknowledgement Size {}", peer_win_ack_size_);
}

// TODO same part
void RTMPSession::command_message_handler() {
  uint8_t *p = complete_read_buf_.read_pos();
  char *command_name;
  int command_name_len;
  AmfOp::decode_string_with_type(p, complete_read_buf_.readable_size(), &command_name, &command_name_len, nullptr);
  if (strncmp(command_name, "connect", 7) == 0) {
    connect_handler();
  } else if (strncmp(command_name, "releaseStream", 13) == 0) {
    release_stream_handler();
  } else if (strncmp(command_name, "FCPublish", 9) == 0) {
    fcpublish_handler();
  } else if (strncmp(command_name, "createStream", 12) == 0) {
    create_stream_handler();
  } else if (strncmp(command_name, "publish", 7) == 0) {
    publish_handler();
  } else if (strncmp(command_name, "FCSubscribe", 11) == 0) {
    fcsubscribe_handler();
  } else if (strncmp(command_name, "play", 4) == 0) {
    play_handler();
  } else {
    YET_LOG_ASSERT(0, "unhandle command:{}", std::string(command_name, command_name_len));
  }
}

void RTMPSession::fcpublish_handler() {
  uint8_t *p = complete_read_buf_.read_pos() + 12;
  double transaction_id;
  p = AmfOp::decode_number_with_type(p, complete_read_buf_.readable_size(), &transaction_id, nullptr);
  if (transaction_id != TRANSACTION_ID_FC_PUBLISH) {
    YET_LOG_ERROR("CHEFFUCKME.");
  }
  p++;
  char *name;
  int name_len;
  AmfOp::decode_string_with_type(p, complete_read_buf_.readable_size(), &name, &name_len, nullptr);
  YET_LOG_INFO("---->FCPublish(\'{}\')", std::string(name, name_len));

}

void RTMPSession::release_stream_handler() {
  uint8_t *p = complete_read_buf_.read_pos() + 16;
  double transaction_id;
  p = AmfOp::decode_number_with_type(p, complete_read_buf_.readable_size(), &transaction_id, nullptr);
  if (transaction_id != TRANSACTION_ID_RELEASE_STREAM) {
    YET_LOG_ERROR("CHEFFUCKME.");
  }
  p++;
  char *name;
  int name_len;
  AmfOp::decode_string_with_type(p, complete_read_buf_.readable_size(), &name, &name_len, nullptr);
  YET_LOG_INFO("---->releaseStream(\'{}\')", std::string(name, name_len));
}

void RTMPSession::fcsubscribe_handler() {
  YET_LOG_INFO("----->FCSubscribe()");
  YET_LOG_WARN("TODO");
}

void RTMPSession::play_handler() {
  uint8_t *p = complete_read_buf_.read_pos() + 7;
  double transaction_id;
  p = AmfOp::decode_number_with_type(p, complete_read_buf_.readable_size(), &transaction_id, nullptr);
  YET_LOG_ERROR("tid:{}", transaction_id);
  p++; // skip null
  char *name;
  int name_len;
  p = AmfOp::decode_string_with_type(p, complete_read_buf_.readable_size(), &name, &name_len, nullptr);
  YET_LOG_INFO("---->play(\'{}\')", std::string(name, name_len));

  // TODO
  // start duration reset
}

void RTMPSession::publish_handler() {
  uint8_t *p = complete_read_buf_.read_pos() + 10;
  double transaction_id;
  p = AmfOp::decode_number_with_type(p, complete_read_buf_.readable_size(), &transaction_id, nullptr);
  if (transaction_id != TRANSACTION_ID_PUBLISH) {
    YET_LOG_ERROR("CHEFFUCKME.");
  }
  p++; // skip null
  char *publishing_name;
  int publishing_name_len;
  p = AmfOp::decode_string_with_type(p, complete_read_buf_.readable_size(), &publishing_name, &publishing_name_len, nullptr);
  char *publishing_type;
  int publishing_type_len;
  p = AmfOp::decode_string_with_type(p, complete_read_buf_.readable_size(), &publishing_type, &publishing_type_len, nullptr);
  YET_LOG_INFO("---->publish(\'{}\')", std::string(publishing_name, publishing_name_len));

  do_write_on_status_publish();
}

void RTMPSession::do_write_on_status_publish() {
  int len = RtmpPackOp::encode_rtmp_msg_on_status_publish_reserve();
  write_buf_.reserve(len);
  // TODO stream id
  RtmpPackOp::encode_on_status_publish(write_buf_.write_pos(), 1);
  YET_LOG_INFO("<----onStatus(\'NetStream.Publish.Start\')");
  SNIPPET_ASYNC_WRITE(write_buf_.read_pos(), len, &RTMPSession::write_on_status_cb);
}

void RTMPSession::write_on_status_cb(ErrorCode ec, std::size_t len) {
  SNIPPET_ENTER_CB;
}

void RTMPSession::create_stream_handler() {
  uint8_t *p = complete_read_buf_.read_pos() + 15;
  p = AmfOp::decode_number_with_type(p, complete_read_buf_.readable_size(), &create_stream_transaction_id_, nullptr);
  //if (transaction_id != TRANSACTION_ID_CREATE_STREAM) {
  //  YET_LOG_ERROR("CHEFFUCKME. {}", transaction_id);
  //}

  // TODO null obj

  YET_LOG_INFO("---->createStream()");

  do_write_create_stream_result();
}

void RTMPSession::do_write_create_stream_result() {
  int len = RtmpPackOp::encode_rtmp_msg_create_stream_result_reserve();
  write_buf_.reserve(len);
  // TODO stream id
  RtmpPackOp::encode_create_stream_result(write_buf_.write_pos(), create_stream_transaction_id_, 1);
  YET_LOG_INFO("<----_result()");
  SNIPPET_ASYNC_WRITE(write_buf_.read_pos(), len, &RTMPSession::write_create_stream_result_cb);
}

void RTMPSession::write_create_stream_result_cb(ErrorCode ec, std::size_t len) {
  SNIPPET_ENTER_CB;
}

void RTMPSession::connect_handler() {
  uint8_t *p = complete_read_buf_.read_pos() + 10;
  double transaction_id;
  p = AmfOp::decode_number_with_type(p, complete_read_buf_.readable_size(), &transaction_id, nullptr);
  if (transaction_id != TRANSACTION_ID_CONNECT) {
    YET_LOG_ERROR("CHEFFUCKME.");
  }

  AmfObjectItemMap objs;
  //YET_LOG_INFO("{}", chef::stuff_op::bytes_to_hex((const uint8_t *)complete_read_buf_.read_pos()+4, 197-4, 16));
  p = AmfOp::decode_object(p, complete_read_buf_.readable_size(), &objs, nullptr);
  YET_LOG_ASSERT(p, "decode command connect failed.");
  //YET_LOG_INFO("transaction_id:{}, objs:{}, decode object succ:{}", transaction_id, objs.stringify(), p != nullptr);

  AmfObjectItem *app = objs.get("app");
  if (app->is_string()) {
    YET_LOG_INFO("---->connect(\'{}\')", app->get_string());
  }

  do_write_win_ack_size();
}

void RTMPSession::do_write_win_ack_size() {
  int len = RtmpPackOp::encode_rtmp_msg_win_ack_size_reserve();
  write_buf_.reserve(len);
  RtmpPackOp::encode_win_ack_size(write_buf_.write_pos(), WINDOW_ACKNOWLEDGEMENT_SIZE);
  YET_LOG_INFO("<----Window Acknowledgement Size {}", WINDOW_ACKNOWLEDGEMENT_SIZE);
  SNIPPET_ASYNC_WRITE(write_buf_.read_pos(), len, &RTMPSession::write_win_ack_size_cb);
}

void RTMPSession::write_win_ack_size_cb(ErrorCode ec, std::size_t len) {
  SNIPPET_ENTER_CB;
  do_write_peer_bandwidth();
}

void RTMPSession::do_write_peer_bandwidth() {
  int len = RtmpPackOp::encode_rtmp_msg_peer_bandwidth_reserve();
  write_buf_.reserve(len);
  RtmpPackOp::encode_peer_bandwidth(write_buf_.write_pos(), PEER_BANDWIDTH);
  YET_LOG_INFO("<----Set Peer Bandwidth {},Dynamic", PEER_BANDWIDTH);
  SNIPPET_ASYNC_WRITE(write_buf_.read_pos(), len, &RTMPSession::write_peer_bandwidth_cb);
}

void RTMPSession::write_peer_bandwidth_cb(ErrorCode ec, std::size_t len) {
  SNIPPET_ENTER_CB;
  do_write_chunk_size();
}

void RTMPSession::do_write_chunk_size() {
  int len = RtmpPackOp::encode_rtmp_msg_chunk_size_reserve();
  write_buf_.reserve(len);
  RtmpPackOp::encode_chunk_size(write_buf_.write_pos(), LOCAL_CHUNK_SIZE);
  YET_LOG_INFO("<----Set Chunk Size {}", LOCAL_CHUNK_SIZE);
  SNIPPET_ASYNC_WRITE(write_buf_.read_pos(), len, &RTMPSession::write_chunk_size_cb);
}

void RTMPSession::write_chunk_size_cb(ErrorCode ec, std::size_t len) {
  SNIPPET_ENTER_CB;
  do_write_connect_result();
}

void RTMPSession::do_write_connect_result() {
  int len = RtmpPackOp::encode_rtmp_msg_connect_result_reserve();
  write_buf_.reserve(len);
  RtmpPackOp::encode_connect_result(write_buf_.write_pos());
  YET_LOG_INFO("<----_result(\'NetConnection.Connect.Success\')");
  SNIPPET_ASYNC_WRITE(write_buf_.read_pos(), len, &RTMPSession::write_connect_result_cb);
}

void RTMPSession::write_connect_result_cb(ErrorCode ec, std::size_t len) {
  SNIPPET_ENTER_CB;
}

} // namespace yet
