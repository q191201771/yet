/**
 * @file   yet_rtmp_handshake.h
 * @author pengrl
 *
 */

#pragma once

#include "yet_common/yet_common.hpp"
#include "yet_rtmp/yet_rtmp.hpp"

namespace yet {

class RtmpHandshakeS;
class RtmpHandshakeC;

class RtmpHandshakeS {
  public:
    bool handle_c0c1(const uint8_t *c0c1, size_t len);
    // CHEFTODO merge s0s1s2
    //uint8_t *create_s0s1s2();
    uint8_t *create_s0s1();
    uint8_t *create_s2();

    bool handle_c2(const uint8_t *, size_t) { return true; }

  private:
    bool rtmp_handshake_parse_challenge(const uint8_t *buf, size_t buf_len,
                                        const uint8_t *peer_key, size_t peer_key_len,
                                        const uint8_t *key, size_t key_len);

    bool rtmp_handshake_create_challenge(uint8_t *buf, size_t buf_len, const uint8_t *version,
                                         const uint8_t *key, size_t key_len);

  public:
    RtmpHandshakeS() {}

  private:
    RtmpHandshakeS(const RtmpHandshakeS &) = delete;
    const RtmpHandshakeS &operator=(const RtmpHandshakeS &) = delete;

  private:
    uint8_t s0s1_[RTMP_S0S1_LEN];
    uint8_t s2_[RTMP_S2_LEN];
    int timestamp_recvd_c1_;
    int timestamp_sent_s1_;
    bool is_old_;
};

// CHEFTODO option to new style?
class RtmpHandshakeC {
  public:
    uint8_t *create_c0c1();
    bool handle_s0s1s2(uint8_t *s0s1s2);
    uint8_t *create_c2();

  private:
    uint8_t c0c1_[RTMP_C0C1_LEN];
    uint8_t c2_[RTMP_C2_LEN];
};

}
