#include "yet_rtmp_pack_op.h"
#include "yet_rtmp/yet_rtmp_amf_op.h"
#include "yet_rtmp/yet_rtmp.hpp"
#include <arpa/inet.h>
#include <string.h>


#define MEMSET_AND_SHIFT(out, val, size) memset(out, val, size);out+=size;
#define MEMSET_ZERO_AND_SHIFT(out, size) MEMSET_AND_SHIFT(out, 0x0, size);

#define STRLEN(s) strlen(s)

namespace yet {

int RtmpPackOp::encode_rtmp_msg_connect_reserve(const char *app, const char *swf_url, const char *tc_url) {
  // 10 -> "connect", 9 -> Number, 1 -> Obj begin, 3 -> Obj end
  return CHUNK_HEADER_SIZE_TYPE0 + 10 + 9 + 1 +
      (app ? AmfOp::encode_object_named_string_reserve(3, STRLEN(app)) : 0) +
      (swf_url ? AmfOp::encode_object_named_string_reserve(6, STRLEN(swf_url)) : 0) +
      (tc_url ? AmfOp::encode_object_named_string_reserve(5, STRLEN(tc_url)) : 0) +
      3;
}

int RtmpPackOp::encode_rtmp_msg_release_stream_reserve(const char *stream_name) {
  // 16 -> "releaseStream", 0 -> Number, 1 -> Null
  return CHUNK_HEADER_SIZE_TYPE0 + 16 + 9 + 1 + AmfOp::encode_string_reserve(STRLEN(stream_name));
}

int RtmpPackOp::encode_rtmp_msg_fc_publish_reserve(const char *stream_name) {
  // 12 -> "FCPublish", 0 -> Number, 1 -> Null
  return CHUNK_HEADER_SIZE_TYPE0 + 12 + 9 + 1 + AmfOp::encode_string_reserve(STRLEN(stream_name));
}

int RtmpPackOp::encode_rtmp_msg_publish_reserve(const char *app, const char *stream_name) {
  // 10 -> "publish", 9 -> Number, 1 -> Null
  return CHUNK_HEADER_SIZE_TYPE0 + 10 + 9 + 1 + AmfOp::encode_string_reserve(STRLEN(app)) +
         AmfOp::encode_string_reserve(STRLEN(stream_name));
}

int RtmpPackOp::encode_rtmp_msg_play_reserve(const char *stream_name) {
  // 7 -> "play", 9 -> Number, 1 -> Null
  return CHUNK_HEADER_SIZE_TYPE0 + 7 + 9 + 1 + AmfOp::encode_string_reserve(STRLEN(stream_name));
}

static inline uint8_t *ENCODE_MESSAGE_HEADER(uint8_t *out, int csid, int len, int type_id, int stream_id) {
  // chunk basic header
  *out++ = (0x0 << 6) | csid; // fmt and chunk stream id

  // chunk message header
  MEMSET_ZERO_AND_SHIFT(out, 3); // timestamp
  out = AmfOp::encode_int24(out, len); // body length(without header)
  *out++ = type_id;
  return AmfOp::encode_int32_le(out, stream_id); // message stream id
}

uint8_t *RtmpPackOp::encode_chunk_size(uint8_t *out, uint32_t cs) {
  out = ENCODE_MESSAGE_HEADER(out, RTMP_CSID_PROTOCOL_CONTROL, 4, RTMP_MSG_TYPE_ID_SET_CHUNK_SIZE, 0);
  out = AmfOp::encode_int32(out, cs);
  return out;
}

uint8_t *RtmpPackOp::encode_win_ack_size(uint8_t *out, int val) {
  out = ENCODE_MESSAGE_HEADER(out, RTMP_CSID_PROTOCOL_CONTROL, 4, RTMP_MSG_TYPE_ID_WIN_ACK_SIZE, 0);
  out = AmfOp::encode_int32(out, val);
  return out;
}

uint8_t *RtmpPackOp::encode_peer_bandwidth(uint8_t *out, int val) {
  out = ENCODE_MESSAGE_HEADER(out, RTMP_CSID_PROTOCOL_CONTROL, 5, RTMP_MSG_TYPE_ID_BANDWIDTH, 0);
  out = AmfOp::encode_int32(out, val);
  *out++ = RTMP_PEER_BANDWITH_LIMIT_TYPE_DYNAMIC;

  return out;
}

uint8_t *RtmpPackOp::encode_user_control_ping_response(uint8_t *out, int timestamp) {
  out = ENCODE_MESSAGE_HEADER(out, RTMP_CSID_PROTOCOL_CONTROL, 6, RTMP_MSG_TYPE_ID_USER_CONTROL, 0);
  out = AmfOp::encode_int16(out, USER_CONTROL_EVENT_TYPE_PING_RESPONSE);
  out = AmfOp::encode_int32(out, timestamp);
  return out;
}

uint8_t *RtmpPackOp::encode_user_control_stream_begin(uint8_t *out) {
  out = ENCODE_MESSAGE_HEADER(out, RTMP_CSID_PROTOCOL_CONTROL, 2, RTMP_MSG_TYPE_ID_USER_CONTROL, 0);
  out = AmfOp::encode_int16(out, USER_CONTROL_EVENT_TYPE_STREAM_BEGIN);
  return out;
}

uint8_t *RtmpPackOp::encode_user_control_stream_eof(uint8_t *out) {
  out = ENCODE_MESSAGE_HEADER(out, RTMP_CSID_PROTOCOL_CONTROL, 2, RTMP_MSG_TYPE_ID_USER_CONTROL, 0);
  out = AmfOp::encode_int16(out, USER_CONTROL_EVENT_TYPE_STREAM_EOF);
  return out;
}

uint8_t *RtmpPackOp::encode_connect(uint8_t *out, int len, const char *app, const char *swf_url, const char *tc_url, uint32_t tid) {
  out = ENCODE_MESSAGE_HEADER(out, RTMP_CSID_OVER_CONNECTION, len-CHUNK_HEADER_SIZE_TYPE0, RTMP_MSG_TYPE_ID_COMMAND_MESSAGE_AMF0, 0);

  out = AmfOp::encode_string(out, "connect", 7);
  out = AmfOp::encode_number(out, tid);
  out = AmfOp::encode_object_begin(out);
  out = AmfOp::encode_object_named_string(out, "app", 3, app, STRLEN(app));
  out = AmfOp::encode_object_named_string(out, "swfUrl", 6, swf_url, STRLEN(swf_url));
  out = AmfOp::encode_object_named_string(out, "tcUrl", 5, tc_url, STRLEN(tc_url));
  out = AmfOp::encode_object_end(out);

  return out;
}

uint8_t *RtmpPackOp::encode_connect_result(uint8_t *out, uint32_t tid) {
  out = ENCODE_MESSAGE_HEADER(out, RTMP_CSID_OVER_CONNECTION, 190, RTMP_MSG_TYPE_ID_COMMAND_MESSAGE_AMF0, 0);

  out = AmfOp::encode_string(out, "_result", 7);
  out = AmfOp::encode_number(out, tid);
  out = AmfOp::encode_object_begin(out);
  // TODO cache me
  out = AmfOp::encode_object_named_string(out, "fmsVer", 6, "FMS/3,0,1,123", 13);
  out = AmfOp::encode_object_named_number(out, "capabilities", 12, 31);
  out = AmfOp::encode_object_end(out);

  out = AmfOp::encode_object_begin(out);
  out = AmfOp::encode_object_named_string(out, "level", 5, "status", 6);
  out = AmfOp::encode_object_named_string(out, "code", 4, "NetConnection.Connect.Success", 29);
  out = AmfOp::encode_object_named_string(out, "description", 11, "Connection succeeded.", 21);
  out = AmfOp::encode_object_named_number(out, "objectEncoding", 14, 0);
  out = AmfOp::encode_object_end(out);

  return out;
}

uint8_t *RtmpPackOp::encode_release_stream(uint8_t *out, int len, const char *stream_name, uint32_t tid) {
  out = ENCODE_MESSAGE_HEADER(out, RTMP_CSID_OVER_CONNECTION, len-CHUNK_HEADER_SIZE_TYPE0, RTMP_MSG_TYPE_ID_COMMAND_MESSAGE_AMF0, 0);

  // TODO cache me
  out = AmfOp::encode_string(out, "releaseStream", 13); // STRLEN("releaseStream")
  out = AmfOp::encode_number(out, tid);
  out = AmfOp::encode_null(out);
  out = AmfOp::encode_string(out, stream_name, STRLEN(stream_name));

  return out;
}

uint8_t *RtmpPackOp::encode_fc_publish(uint8_t *out, int len, const char *stream_name, uint32_t tid) {
  out = ENCODE_MESSAGE_HEADER(out, RTMP_CSID_OVER_CONNECTION, len-CHUNK_HEADER_SIZE_TYPE0, RTMP_MSG_TYPE_ID_COMMAND_MESSAGE_AMF0, 0);

  out = AmfOp::encode_string(out, "FCPublish", 9); // STRLEN("FCPublish")
  out = AmfOp::encode_number(out, tid);
  out = AmfOp::encode_null(out);
  out = AmfOp::encode_string(out, stream_name, STRLEN(stream_name));

  return out;
}


uint8_t *RtmpPackOp::encode_create_stream(uint8_t *out, uint32_t tid) {
  out = ENCODE_MESSAGE_HEADER(out, RTMP_CSID_OVER_CONNECTION,
                              ENCODE_RTMP_MSG_CREATE_STREAM_RESERVE-CHUNK_HEADER_SIZE_TYPE0, RTMP_MSG_TYPE_ID_COMMAND_MESSAGE_AMF0, 0);

  out = AmfOp::encode_string(out, "createStream", 12); // STRLEN("createStream")
  out = AmfOp::encode_number(out, tid);
  out = AmfOp::encode_null(out);

  return out;
}

uint8_t *RtmpPackOp::encode_create_stream_result(uint8_t *out, int tid) {
  out = ENCODE_MESSAGE_HEADER(out, RTMP_CSID_OVER_CONNECTION, 29, RTMP_MSG_TYPE_ID_COMMAND_MESSAGE_AMF0, 0);
  out = AmfOp::encode_string(out, "_result", 7);
  out = AmfOp::encode_number(out, tid);
  out = AmfOp::encode_null(out);
  out = AmfOp::encode_number(out, RTMP_MSID);
  return out;
}

uint8_t *RtmpPackOp::encode_on_status_publish(uint8_t *out, int stream_id) {
  out = ENCODE_MESSAGE_HEADER(out, RTMP_CSID_OVER_STREAM, 105, RTMP_MSG_TYPE_ID_COMMAND_MESSAGE_AMF0, stream_id);
  out = AmfOp::encode_string(out, "onStatus", 8);
  out = AmfOp::encode_number(out, 0);
  out = AmfOp::encode_null(out);

  out = AmfOp::encode_object_begin(out);
  out = AmfOp::encode_object_named_string(out, "level", 5, "status", 6);
  out = AmfOp::encode_object_named_string(out, "code", 4, "NetStream.Publish.Start", 23);
  out = AmfOp::encode_object_named_string(out, "description", 11, "Start publishing", 16);
  out = AmfOp::encode_object_end(out);
  return out;
}

uint8_t *RtmpPackOp::encode_on_status_play(uint8_t *out, int stream_id) {
  out = ENCODE_MESSAGE_HEADER(out, RTMP_CSID_OVER_STREAM, 96, RTMP_MSG_TYPE_ID_COMMAND_MESSAGE_AMF0, stream_id);
  out = AmfOp::encode_string(out, "onStatus", 8);
  out = AmfOp::encode_number(out, 0);
  out = AmfOp::encode_null(out);

  out = AmfOp::encode_object_begin(out);
  out = AmfOp::encode_object_named_string(out, "level", 5, "status", 6);
  out = AmfOp::encode_object_named_string(out, "code", 4, "NetStream.Play.Start", 20);
  out = AmfOp::encode_object_named_string(out, "description", 11, "Start live", 10);
  out = AmfOp::encode_object_end(out);
  return out;
}

uint8_t *RtmpPackOp::encode_publish(uint8_t *out, int len, const char *app, const char *stream_name, int stream_id, uint32_t tid) {
  out = ENCODE_MESSAGE_HEADER(out, RTMP_CSID_OVER_STREAM, len-CHUNK_HEADER_SIZE_TYPE0, RTMP_MSG_TYPE_ID_COMMAND_MESSAGE_AMF0, stream_id);

  out = AmfOp::encode_string(out, "publish", 7); // STRLEN("publish")
  out = AmfOp::encode_number(out, tid);
  out = AmfOp::encode_null(out);
  out = AmfOp::encode_string(out, stream_name, STRLEN(stream_name));
  out = AmfOp::encode_string(out, app, STRLEN(app));

  return out;
}

uint8_t *RtmpPackOp::encode_play(uint8_t *out, int len, const char *stream_name, int stream_id, uint32_t tid) {
  out = ENCODE_MESSAGE_HEADER(out, RTMP_CSID_OVER_STREAM, len-CHUNK_HEADER_SIZE_TYPE0, RTMP_MSG_TYPE_ID_COMMAND_MESSAGE_AMF0, stream_id);

  out = AmfOp::encode_string(out, "play", 4);
  out = AmfOp::encode_number(out, tid);
  out = AmfOp::encode_null(out);
  out = AmfOp::encode_string(out, stream_name, STRLEN(stream_name));

  return out;
}

}; // namespace yet
