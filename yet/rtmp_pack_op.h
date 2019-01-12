/**
 * @file   rtmp_pack_op.h
 * @author pengrl
 *
 */

#pragma once

#include <inttypes.h>

namespace yet {

  static constexpr int CHUNK_HEADER_SIZE_TYPE0 = 12;

  static constexpr int USER_CONTROL_EVENT_TYPE_PING_REQUEST = 6;
  static constexpr int USER_CONTROL_EVENT_TYPE_PING_RESPONSE = 7;

  class RtmpPackOp {
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
      static uint8_t *encode_win_ack_size(uint8_t *out, int val);
      static uint8_t *encode_chunk_size(uint8_t *out, uint32_t cs);
      static uint8_t *encode_user_control_ping_response(uint8_t *out, int timestamp);
      static uint8_t *encode_connect(uint8_t *out, int len, const char *app, const char *swf_url, const char *tc_url);
      static uint8_t *encode_release_stream(uint8_t *out, int len, const char *stream_name);
      static uint8_t *encode_fc_publish(uint8_t *out, int len, const char *stream_name);
      static uint8_t *encode_create_stream(uint8_t *out);
      static uint8_t *encode_publish(uint8_t *out, int len, const char *app, const char *stream_name, int stream_id);

      static uint8_t *encode_connect_result(uint8_t *out);
      static uint8_t *encode_peer_bandwidth(uint8_t *out, int val);
      static uint8_t *encode_create_stream_result(uint8_t *out, int transaction_id, int stream_id);
      static uint8_t *encode_on_status_publish(uint8_t *out, int stream_id);

    private:
      RtmpPackOp() = delete;
      RtmpPackOp(const RtmpPackOp &) = delete;
      const RtmpPackOp &operator=(const RtmpPackOp &) = delete;

  }; // class RtmpPackOp

}; // namespace cav
