/**
 * @file   yet_rtmp_handshake.h
 * @author pengrl
 *
 */

#pragma once

#include "yet_common/yet_common.hpp"
#include "yet_rtmp/yet_rtmp.hpp"

namespace yet {

class RtmpHandshake {
  public:
    bool handle_c0c1(const uint8_t *c0c1, std::size_t len);
    uint8_t *create_s0s1();
    uint8_t *create_s2();

    //uint8_t *create_s0s1s2();
    bool handle_c2(const uint8_t *, std::size_t) { return true; }

  private:
    bool rtmp_handshake_parse_challenge(const uint8_t *buf, std::size_t buf_len,
                                        const uint8_t *peer_key, std::size_t peer_key_len,
                                        const uint8_t *key, std::size_t key_len);

    bool rtmp_handshake_create_challenge(uint8_t *buf, std::size_t buf_len, const uint8_t *version,
                                         const uint8_t *key, std::size_t key_len);

  public:
    RtmpHandshake() {}

  private:
    RtmpHandshake(const RtmpHandshake &) = delete;
    const RtmpHandshake &operator=(const RtmpHandshake &) = delete;

  private:
    uint8_t s0s1_[RTMP_S0S1_LEN];
    uint8_t s2_[RTMP_S2_LEN];
    int timestamp_recvd_c1_;
    int timestamp_sent_s1_;
    bool is_old_;
};

}
