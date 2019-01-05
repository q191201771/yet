/**
 * @file   rtmp_pack.h
 * @author pengrl
 *
 */

#pragma once

#include <stdint.h>


namespace yet {

  static constexpr int CHUNK_HEADER_SIZE_TYPE0 = 12;

  static constexpr int USER_CONTROL_EVENT_TYPE_PING_REQUEST = 6;
  static constexpr int USER_CONTROL_EVENT_TYPE_PING_RESPONSE = 7;

  class RtmpPack {
    public:
      static constexpr int ENCODE_RTMP_MSG_WIN_ACK_SIZE_RESERVE         = 16;  // 12 + 4
      static constexpr int ENCODE_RTMP_MSG_CHUNK_SIZE_RESERVE           = 16;  // 12 + 4
      static constexpr int ENCODE_RTMP_MSG_CREATE_STREAM_RESERVE        = 37;  // 12 + 15 + 9 + 1
      static constexpr int ENCODE_RTMP_MSG_USER_CONTORL_PING_RESPONSE   = 18;  // 12 + 2 + 4
      static constexpr int ENCODE_RTMP_MSG_CONNECT_RESULT_RESERVE       = 202; // 12 + 190
      static constexpr int ENCODE_RTMP_MSG_PEER_BANDWIDTH               = 17;  // 12 + 4 + 1
      static constexpr int ENCODE_RTMP_MSG_CREATE_STREAM_RESULT_RESERVE = 41;  // 12 + 10 + 9 + 1 + 9
      static constexpr int ENCODE_RTMP_MSG_ON_STATUS_PUBLISH_RESERVE    = 117; // 12 + 105

      static int encode_rtmp_msg_win_ack_size_reserve() { return ENCODE_RTMP_MSG_WIN_ACK_SIZE_RESERVE; }
      static int encode_rtmp_msg_chunk_size_reserve() { return ENCODE_RTMP_MSG_CHUNK_SIZE_RESERVE; }
      static int encode_rtmp_msg_connect_reserve(const char *app, const char *swf_url, const char *tc_url);
      static int encode_rtmp_msg_release_stream_reserve(const char *stream_name);
      static int encode_rtmp_msg_fc_publish_reserve(const char *stream_name);
      static int encode_rtmp_msg_create_stream_reserve() { return ENCODE_RTMP_MSG_CREATE_STREAM_RESERVE; }
      static int encode_rtmp_msg_publish_reserve(const char *app, const char *stream_name);

      static int encode_rtmp_msg_connect_result_reserve() { return ENCODE_RTMP_MSG_CONNECT_RESULT_RESERVE; }
      static int encode_rtmp_msg_peer_bandwidth_reserve() { return ENCODE_RTMP_MSG_PEER_BANDWIDTH; }
      static int encode_rtmp_msg_create_stream_result_reserve() { return ENCODE_RTMP_MSG_CREATE_STREAM_RESULT_RESERVE; }
      static int encode_rtmp_msg_on_status_publish_reserve() { return ENCODE_RTMP_MSG_ON_STATUS_PUBLISH_RESERVE; }

    public:
      // 空间由外部申请
      static char *encode_win_ack_size(char *out, int val);
      static char *encode_chunk_size(char *out, uint32_t cs);
      static char *encode_user_control_ping_response(char *out, int timestamp);
      static char *encode_connect(char *out, int len, const char *app, const char *swf_url, const char *tc_url);
      static char *encode_release_stream(char *out, int len, const char *stream_name);
      static char *encode_fc_publish(char *out, int len, const char *stream_name);
      static char *encode_create_stream(char *out);
      static char *encode_publish(char *out, int len, const char *app, const char *stream_name, int stream_id);

      static char *encode_connect_result(char *out);
      static char *encode_peer_bandwidth(char *out, int val);
      static char *encode_create_stream_result(char *out, int stream_id);
      static char *encode_on_status_publish(char *out, int stream_id);

  }; // class RtmpPack

}; // namespace cav
