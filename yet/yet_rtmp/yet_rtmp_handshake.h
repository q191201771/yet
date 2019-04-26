/**
 * @file   yet_rtmp_handshake.h
 * @author pengrl
 *
 */

#pragma once

#include "yet_common/yet_common.hpp"
#include "yet_common/span.hpp"
#include "yet_rtmp/yet_rtmp.hpp"

namespace yet {

class RtmpHandshakeS;
class RtmpHandshakeC;

class RtmpHandshakeS {
  public:
    bool handle_c0c1(nonstd::span<const uint8_t> c0c1);
    // CHEFTODO merge s0s1s2
    //uint8_t *create_s0s1s2();
    uint8_t *create_s0s1();
    uint8_t *create_s2();

    bool handle_c2(nonstd::span<const uint8_t>) { return true; }

  private:
    bool rtmp_handshake_parse_challenge(nonstd::span<const uint8_t> buf,
                                        nonstd::span<const uint8_t> peer_key,
                                        nonstd::span<const uint8_t> key);

    bool rtmp_handshake_create_challenge(nonstd::span<uint8_t> buf, const uint8_t *version,
                                         nonstd::span<const uint8_t> key);

  public:
    RtmpHandshakeS() {}

  private:
    RtmpHandshakeS(const RtmpHandshakeS &) = delete;
    const RtmpHandshakeS &operator=(const RtmpHandshakeS &) = delete;

  private:
    std::array<uint8_t, RTMP_S0S1_LEN> s0s1_;
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
