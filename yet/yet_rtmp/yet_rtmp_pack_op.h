/**
 * @file   yet_rtmp_pack_op.h
 * @author pengrl
 *
 */

#pragma once

#include <inttypes.h>

namespace yet {

  // CHEFTODO  encode by local chunk size
  class RtmpPackOp {
    public:
      static constexpr int ENCODE_WIN_ACK_SIZE_RESERVE               = 16;  // 12 + 4
      static constexpr int ENCODE_CHUNK_SIZE_RESERVE                 = 16;  // 12 + 4
      static constexpr int ENCODE_CREATE_STREAM_RESERVE              = 37;  // 12 + 15 + 9 + 1
      static constexpr int ENCODE_USER_CONTROL_PING_RESPONSE_RESERVE = 18;  // 12 + 2 + 4
      static constexpr int ENCODE_USER_CONTROL_PONG_RESPONSE_RESERVE = 18;  // 12 + 2 + 4
      static constexpr int ENCODE_USER_CONTROL_STREAM_BEGIN_RESERVE  = 14;  // 12 + 2
      static constexpr int ENCODE_USER_CONTROL_STREAM_EOF_RESERVE    = 14;  // 12 + 2
      static constexpr int ENCODE_CONNECT_RESULT_RESERVE             = 202; // 12 + 190
      static constexpr int ENCODE_PEER_BANDWIDTH                     = 17;  // 12 + 4 + 1
      static constexpr int ENCODE_CREATE_STREAM_RESULT_RESERVE       = 41;  // 12 + 10 + 9 + 1 + 9
      static constexpr int ENCODE_ON_STATUS_PUBLISH_RESERVE          = 117; // 12 + 105
      static constexpr int ENCODE_ON_STATUS_PLAY_RESERVE             = 108; // 12 + 96

      static int encode_connect_reserve(const char *app, const char *swf_url, const char *tc_url);
      static int encode_release_stream_reserve(const char *stream_name);
      static int encode_fc_publish_reserve(const char *stream_name);
      static int encode_publish_reserve(const char *app, const char *stream_name);
      static int encode_play_reserve(const char *stream_name);
      static int encode_win_ack_size_reserve() { return ENCODE_WIN_ACK_SIZE_RESERVE; }
      static int encode_chunk_size_reserve() { return ENCODE_CHUNK_SIZE_RESERVE; }
      static int encode_create_stream_reserve() { return ENCODE_CREATE_STREAM_RESERVE; }
      static int encode_connect_result_reserve() { return ENCODE_CONNECT_RESULT_RESERVE; }
      static int encode_peer_bandwidth_reserve() { return ENCODE_PEER_BANDWIDTH; }
      static int encode_create_stream_result_reserve() { return ENCODE_CREATE_STREAM_RESULT_RESERVE; }
      static int encode_on_status_publish_reserve() { return ENCODE_ON_STATUS_PUBLISH_RESERVE; }
      static int encode_on_status_play_reserve() { return ENCODE_ON_STATUS_PLAY_RESERVE; }
      static int encode_user_control_stream_begin_reserve() { return ENCODE_USER_CONTROL_STREAM_BEGIN_RESERVE; }
      static int encode_user_control_stream_eof_reserve() { return ENCODE_USER_CONTROL_STREAM_EOF_RESERVE; }

    public:
      /// memory alloc outsize by <out>
      static uint8_t *encode_win_ack_size(uint8_t *out, int val);
      static uint8_t *encode_chunk_size(uint8_t *out, uint32_t cs);
      static uint8_t *encode_user_control_ping_response(uint8_t *out, int timestamp);
      static uint8_t *encode_user_control_stream_begin(uint8_t *out);
      static uint8_t *encode_user_control_stream_eof(uint8_t *out);
      // CHEFTODO connect with other param
      static uint8_t *encode_connect(uint8_t *out, int len, const char *app, const char *swf_url, const char *tc_url, uint32_t tid);
      static uint8_t *encode_release_stream(uint8_t *out, int len, const char *stream_name, uint32_t tid);
      static uint8_t *encode_fc_publish(uint8_t *out, int len, const char *stream_name, uint32_t tid);
      static uint8_t *encode_create_stream(uint8_t *out, uint32_t tid);
      static uint8_t *encode_publish(uint8_t *out, int len, const char *app, const char *stream_name, int stream_id, uint32_t tid);
      static uint8_t *encode_play(uint8_t *out, int len, const char *stream_name, int stream_id, uint32_t tid);

      static uint8_t *encode_connect_result(uint8_t *out, uint32_t tid);
      static uint8_t *encode_peer_bandwidth(uint8_t *out, int val);
      static uint8_t *encode_create_stream_result(uint8_t *out, int tid);
      static uint8_t *encode_on_status_publish(uint8_t *out, int stream_id);
      static uint8_t *encode_on_status_play(uint8_t *out, int stream_id);

    private:
      RtmpPackOp() = delete;
      RtmpPackOp(const RtmpPackOp &) = delete;
      const RtmpPackOp &operator=(const RtmpPackOp &) = delete;

  }; // class RtmpPackOp

}; // namespace yet
