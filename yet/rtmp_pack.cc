#include "rtmp_pack.h"
#include "rtmp_amf_op.h"
#include "yet_inner.hpp"
#include <arpa/inet.h>
#include <string.h>


#define MEMSET_AND_SHIFT(out, val, size) memset(out, val, size);out+=size;
#define MEMSET_ZERO_AND_SHIFT(out, size) MEMSET_AND_SHIFT(out, 0x0, size);

namespace yet {

int RtmpPack::encode_rtmp_msg_connect_reserve(const char *app, const char *swf_url, const char *tc_url) {
  // 10 -> "connect", 9 -> Number, 1 -> Obj begin, 3 -> Obj end
  return CHUNK_HEADER_SIZE_TYPE0 + 10 + 9 + 1 +
      (app ? AmfOp::encode_object_named_string_reserve(3, strlen(app)) : 0) +
      (swf_url ? AmfOp::encode_object_named_string_reserve(6, strlen(swf_url)) : 0) +
      (tc_url ? AmfOp::encode_object_named_string_reserve(5, strlen(tc_url)) : 0) +
      3;
}

int RtmpPack::encode_rtmp_msg_release_stream_reserve(const char *stream_name) {
  // 16 -> "releaseStream", 0 -> Number, 1 -> Null
  return CHUNK_HEADER_SIZE_TYPE0 + 16 + 9 + 1 + AmfOp::encode_string_reserve(strlen(stream_name));
}

int RtmpPack::encode_rtmp_msg_fc_publish_reserve(const char *stream_name) {
  // 12 -> "FCPublish", 0 -> Number, 1 -> Null
  return CHUNK_HEADER_SIZE_TYPE0 + 12 + 9 + 1 + AmfOp::encode_string_reserve(strlen(stream_name));
}

int RtmpPack::encode_rtmp_msg_publish_reserve(const char *app, const char *stream_name) {
  // 10 -> "publish", 9 -> Number, 1 -> Null
  return CHUNK_HEADER_SIZE_TYPE0 + 10 + 9 + 1 + AmfOp::encode_string_reserve(strlen(app)) +
         AmfOp::encode_string_reserve(strlen(stream_name));
}

static inline char *ENCODE_MESSAGE_HEADER(char *out, int csid, int len, int type_id, int stream_id) {
  // chunk basic header
  *out++ = (0x0 << 6) | csid; // fmt and chunk stream id

  // chunk message header
  MEMSET_ZERO_AND_SHIFT(out, 3); // timestamp
  out = AmfOp::encode_int24(out, len); // body length(without header)
  *out++ = type_id;
  return AmfOp::encode_int32_le(out, stream_id); // message stream id
}

char *RtmpPack::encode_chunk_size(char *out, uint32_t cs) {
  out = ENCODE_MESSAGE_HEADER(out, RTMP_CSID_PROTOCOL_CONTROL, 4, MSG_TYPE_ID_SET_CHUNK_SIZE, 0);
  out = AmfOp::encode_int32(out, cs);
  return out;
}

char *RtmpPack::encode_win_ack_size(char *out, int val) {
  out = ENCODE_MESSAGE_HEADER(out, RTMP_CSID_PROTOCOL_CONTROL, 4, MSG_TYPE_ID_WIN_ACK_SIZE, 0);
  out = AmfOp::encode_int32(out, val);
  return out;
}

char *RtmpPack::encode_peer_bandwidth(char *out, int val) {
  out = ENCODE_MESSAGE_HEADER(out, RTMP_CSID_PROTOCOL_CONTROL, 5, MSG_TYPE_ID_BANDWIDTH, 0);
  out = AmfOp::encode_int32(out, val);
  *out++ = PEER_BANDWITH_LIMIT_TYPE_DYNAMIC;

  return out;
}

char *RtmpPack::encode_user_control_ping_response(char *out, int timestamp) {
  out = ENCODE_MESSAGE_HEADER(out, RTMP_CSID_PROTOCOL_CONTROL, 6, MSG_TYPE_ID_USER_CONTROL, 0);
  out = AmfOp::encode_int16(out, USER_CONTROL_EVENT_TYPE_PING_RESPONSE);
  out = AmfOp::encode_int32(out, timestamp);
  return out;
}

char *RtmpPack::encode_connect(char *out, int len, const char *app, const char *swf_url, const char *tc_url) {
  out = ENCODE_MESSAGE_HEADER(out, RTMP_CSID_OVER_CONNECTION, len-CHUNK_HEADER_SIZE_TYPE0, MSG_TYPE_ID_COMMAND_MESSAGE_AMF0, 0);

  out = AmfOp::encode_string(out, "connect", 7);
  out = AmfOp::encode_number(out, TRANSACTION_ID_CONNECT);
  out = AmfOp::encode_object_begin(out);
  out = AmfOp::encode_object_named_string(out, "app", 3, app, strlen(app));
  out = AmfOp::encode_object_named_string(out, "swfUrl", 6, swf_url, strlen(swf_url));
  out = AmfOp::encode_object_named_string(out, "tcUrl", 5, tc_url, strlen(tc_url));
  out = AmfOp::encode_object_end(out);

  return out;
}

char *RtmpPack::encode_connect_result(char *out) {
  out = ENCODE_MESSAGE_HEADER(out, RTMP_CSID_OVER_CONNECTION, 190, MSG_TYPE_ID_COMMAND_MESSAGE_AMF0, 0);

  out = AmfOp::encode_string(out, "_result", 7);
  out = AmfOp::encode_number(out, TRANSACTION_ID_CONNECT);
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

char *RtmpPack::encode_release_stream(char *out, int len, const char *stream_name) {
  out = ENCODE_MESSAGE_HEADER(out, RTMP_CSID_OVER_CONNECTION, len-CHUNK_HEADER_SIZE_TYPE0, MSG_TYPE_ID_COMMAND_MESSAGE_AMF0, 0);

  // TODO cache me
  out = AmfOp::encode_string(out, "releaseStream", strlen("releaseStream"));
  out = AmfOp::encode_number(out, TRANSACTION_ID_RELEASE_STREAM);
  out = AmfOp::encode_null(out);
  out = AmfOp::encode_string(out, stream_name, strlen(stream_name));

  return out;
}

char *RtmpPack::encode_fc_publish(char *out, int len, const char *stream_name) {
  out = ENCODE_MESSAGE_HEADER(out, RTMP_CSID_OVER_CONNECTION, len-CHUNK_HEADER_SIZE_TYPE0, MSG_TYPE_ID_COMMAND_MESSAGE_AMF0, 0);

  out = AmfOp::encode_string(out, "FCPublish", strlen("FCPublish"));
  out = AmfOp::encode_number(out, TRANSACTION_ID_FC_PUBLISH);
  out = AmfOp::encode_null(out);
  out = AmfOp::encode_string(out, stream_name, strlen(stream_name));

  return out;
}


char *RtmpPack::encode_create_stream(char *out) {
  out = ENCODE_MESSAGE_HEADER(out, RTMP_CSID_OVER_CONNECTION,
                              ENCODE_RTMP_MSG_CREATE_STREAM_RESERVE-CHUNK_HEADER_SIZE_TYPE0, MSG_TYPE_ID_COMMAND_MESSAGE_AMF0, 0);

  out = AmfOp::encode_string(out, "createStream", strlen("createStream"));
  out = AmfOp::encode_number(out, TRANSACTION_ID_CREATE_STREAM);
  out = AmfOp::encode_null(out);

  return out;
}

char *RtmpPack::encode_create_stream_result(char *out, int stream_id) {
  out = ENCODE_MESSAGE_HEADER(out, RTMP_CSID_OVER_CONNECTION, 29, MSG_TYPE_ID_COMMAND_MESSAGE_AMF0, 0);
  out = AmfOp::encode_string(out, "_result", 7);
  out = AmfOp::encode_number(out, TRANSACTION_ID_CREATE_STREAM);
  out = AmfOp::encode_null(out);
  out = AmfOp::encode_number(out, stream_id);
  return out;
}

char *RtmpPack::encode_on_status_publish(char *out, int stream_id) {
  out = ENCODE_MESSAGE_HEADER(out, RTMP_CSID_OVER_STREAM, 105, MSG_TYPE_ID_COMMAND_MESSAGE_AMF0, stream_id);
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

char *RtmpPack::encode_publish(char *out, int len, const char *app, const char *stream_name, int stream_id) {
  out = ENCODE_MESSAGE_HEADER(out, RTMP_CSID_OVER_STREAM, len-CHUNK_HEADER_SIZE_TYPE0, MSG_TYPE_ID_COMMAND_MESSAGE_AMF0, stream_id);

  out = AmfOp::encode_string(out, "publish", strlen("publish"));
  out = AmfOp::encode_number(out, TRANSACTION_ID_PUBLISH);
  out = AmfOp::encode_null(out);
  out = AmfOp::encode_string(out, stream_name, strlen(stream_name));
  out = AmfOp::encode_string(out, app, strlen(app));

  return out;
}

}; // namespace cav
