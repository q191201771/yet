#include "yet_rtmp_handshake.h"
#include "yet_rtmp/yet_rtmp_hmac_sha256_adapter.hpp"
#include "yet_rtmp/yet_rtmp_amf_op.h"
#include "chef_base/chef_stuff_op.hpp"

namespace yet {

// old style simple: obs rtmpdump
// new style complex: ffmpeg vlc

static constexpr size_t RTMP_HANDSHAKE_KEYLEN = 32;

static constexpr uint8_t RTMP_SERVER_VERSION[] = {
    0x0D, 0x0E, 0x0A, 0x0D
};

//static constexpr uint8_t RTMP_CLIENT_VERSION[] = {
//    0x0C, 0x00, 0x0D, 0x0E
//};

static constexpr uint8_t RTMP_CLIENT_KEY[] = {
  'G', 'e', 'n', 'u', 'i', 'n', 'e', ' ', 'A', 'd', 'o', 'b', 'e', ' ',
  'F', 'l', 'a', 's', 'h', ' ', 'P', 'l', 'a', 'y', 'e', 'r', ' ',
  '0', '0', '1',

  0xF0, 0xEE, 0xC2, 0x4A, 0x80, 0x68, 0xBE, 0xE8, 0x2E, 0x00, 0xD0, 0xD1,
  0x02, 0x9E, 0x7E, 0x57, 0x6E, 0xEC, 0x5D, 0x2D, 0x29, 0x80, 0x6F, 0xAB,
  0x93, 0xB8, 0xE6, 0x36, 0xCF, 0xEB, 0x31, 0xAE
};

static constexpr uint8_t RTMP_SERVER_KEY[] = {
  'G', 'e', 'n', 'u', 'i', 'n', 'e', ' ', 'A', 'd', 'o', 'b', 'e', ' ',
  'F', 'l', 'a', 's', 'h', ' ', 'M', 'e', 'd', 'i', 'a', ' ',
  'S', 'e', 'r', 'v', 'e', 'r', ' ',
  '0', '0', '1',

  0xF0, 0xEE, 0xC2, 0x4A, 0x80, 0x68, 0xBE, 0xE8, 0x2E, 0x00, 0xD0, 0xD1,
  0x02, 0x9E, 0x7E, 0x57, 0x6E, 0xEC, 0x5D, 0x2D, 0x29, 0x80, 0x6F, 0xAB,
  0x93, 0xB8, 0xE6, 0x36, 0xCF, 0xEB, 0x31, 0xAE
};

static constexpr size_t RTMP_SERVER_FULL_KEYLEN = sizeof(RTMP_SERVER_KEY);
static constexpr size_t RTMP_SERVER_PART_KEYLEN = 36;
//static constexpr size_t RTMP_CLIENT_FULL_KEYLEN = sizeof(RTMP_CLIENT_KEY);
static constexpr size_t RTMP_CLIENT_PART_KEYLEN = 30;

static bool rtmp_make_digest(nonstd::span<const uint8_t> buf, const uint8_t *skip,
                             nonstd::span<const uint8_t> key,
                             uint8_t *dst)
{
  HMACSHA256 crypto;
  crypto.init(&key[0], key.size());

  if (skip) {
    // left
    if (skip != &buf[0]) { crypto.update(&buf[0], skip-&buf[0]); }

    // right
    auto right_len = &buf[0] + buf.size() - skip - RTMP_HANDSHAKE_KEYLEN;
    if (right_len > 0) { crypto.update(skip + RTMP_HANDSHAKE_KEYLEN, right_len); }

  } else {
    crypto.update(&buf[0], buf.size());
  }

  crypto.final(dst);
  return true;
}

static int rtmp_find_digest(nonstd::span<const uint8_t> buf, size_t base, nonstd::span<const uint8_t> key) {
  auto offs = buf[base] + buf[base+1] + buf[base+2] + buf[base+3];
  offs = (offs % 728) + base + 4;
  auto skip = &buf[offs];

  uint8_t digest[RTMP_HANDSHAKE_KEYLEN];
  if (!rtmp_make_digest(buf, skip, key, digest)) { return -1; }

  return (memcmp(digest, skip, RTMP_HANDSHAKE_KEYLEN) == 0) ? offs : -1;
}

bool RtmpHandshakeS::rtmp_handshake_create_challenge(nonstd::span<uint8_t> buf, const uint8_t *version,
                                                     nonstd::span<const uint8_t> key)
{
  buf[0] = RTMP_VERSION;
  buf = buf.subspan(1);
  timestamp_sent_s1_ = static_cast<int>(chef::stuff_op::unix_timestamp_msec() / 1000);
  AmfOp::encode_int32(&buf[0], timestamp_sent_s1_);
  memcpy(&buf[4], version, 4);
  // CHEFTODO random

  auto  offs = buf[8] + buf[9] + buf[10] + buf[11];
  offs = (offs % 728) + 12;
  auto *skip = &buf[offs];

  return rtmp_make_digest(buf, skip, key, skip);
}

bool RtmpHandshakeS::rtmp_handshake_parse_challenge(nonstd::span<const uint8_t> buf,
                                                    nonstd::span<const uint8_t> peer_key,
                                                    nonstd::span<const uint8_t> key)
{
  if (buf[0] != RTMP_VERSION) {
    YET_LOG_ERROR("Handle c0c1 failed since version invalid. ver:{}", buf[0]);
    return false;
  }
  buf = buf.subspan(1);

  int peer_epoch;
  AmfOp::decode_int32(&buf[0], 4, &peer_epoch, nullptr);
  int ver;
  AmfOp::decode_int32(&buf[4], 4, &ver, nullptr);
  if (ver == 0) {
    YET_LOG_INFO("Rtmp handshake old style.");
    is_old_ = true;
    return true;
  }
  // key before digest
  auto offs = rtmp_find_digest(buf, 764+8, peer_key);
  if (offs == -1) {
    // digest before key
    offs = rtmp_find_digest(buf, 8, peer_key);
  }
  if (offs == -1) {
    YET_LOG_INFO("Rtmp handshake old style.");
    is_old_ = true;
    return true;
  }

  YET_LOG_INFO("Rtmp handshake new style. offs:{}", offs);
  is_old_ = false;
  // CHEFTODO random

  uint8_t digest[RTMP_HANDSHAKE_KEYLEN];
  if (!rtmp_make_digest({&buf[offs], RTMP_HANDSHAKE_KEYLEN}, nullptr, key, digest)) {
    YET_LOG_ERROR("CHEFGREPME Make s2 digest failed.");
    return false;
  }

  static constexpr size_t digest_pos = RTMP_S2_LEN - RTMP_HANDSHAKE_KEYLEN;
  if (!rtmp_make_digest({s2_, RTMP_S2_LEN}, (const uint8_t *)s2_+digest_pos, {digest, RTMP_HANDSHAKE_KEYLEN}, (uint8_t *)s2_+ digest_pos)) {
    YET_LOG_ERROR("CHEFGREPME Make s2 final digest failed.");
    return false;
  }

  return true;
}

bool RtmpHandshakeS::handle_c0c1(nonstd::span<const uint8_t> c0c1) {
  if (c0c1.size() < RTMP_C0C1_LEN) {
    YET_LOG_ERROR("Handle c0c1 failed since len too short. len:{}", c0c1.size());
    return false;
  }

  rtmp_handshake_parse_challenge(c0c1, {RTMP_CLIENT_KEY, RTMP_CLIENT_PART_KEYLEN}, {RTMP_SERVER_KEY, RTMP_SERVER_FULL_KEYLEN});

  if (is_old_) {
    AmfOp::decode_int32(&c0c1[1], c0c1.size() - 1, &timestamp_recvd_c1_, nullptr);
    memcpy(s2_, &c0c1[1], c0c1.size() - 1);
  }
  return true;
}

uint8_t *RtmpHandshakeS::create_s0s1() {
  if (is_old_) {
    s0s1_[0] = RTMP_VERSION;
    timestamp_sent_s1_ = static_cast<int>(chef::stuff_op::unix_timestamp_msec() / 1000);
    AmfOp::encode_int32(&s0s1_[1], timestamp_sent_s1_);
    memset(&s0s1_[5], 0, 4);

    // CHEFTODO random 1528

    return &s0s1_[0];
  }

  rtmp_handshake_create_challenge(s0s1_, RTMP_SERVER_VERSION, { RTMP_SERVER_KEY, RTMP_SERVER_PART_KEYLEN });
  return &s0s1_[0];
}

uint8_t *RtmpHandshakeS::create_s2() {
  if (is_old_) {
    AmfOp::encode_int32(s2_, timestamp_recvd_c1_);
    AmfOp::encode_int32(s2_+4, timestamp_sent_s1_);
    return s2_;
  }

  return s2_;
}

uint8_t *RtmpHandshakeC::create_c0c1() {
  c0c1_[0] = RTMP_VERSION;
  auto now = static_cast<int>(chef::stuff_op::unix_timestamp_msec() / 1000);
  AmfOp::encode_int32(c0c1_+1, now);
  memset(c0c1_+5, 0, 4);
  // hack in yet watermark
  c0c1_[9] = 'y'; c0c1_[10] = 'e'; c0c1_[11] = 't';
  // CHEFTODO random

  return c0c1_;
}

bool RtmpHandshakeC::handle_s0s1s2(uint8_t *s0s1s2) {
  if (s0s1s2[0] != RTMP_VERSION) { return false; }

  if (memcmp(c0c1_ + 9, s0s1s2 + RTMP_C0C1_LEN + 8, RTMP_C0C1_LEN - 9) != 0) { return false; }

  memcpy(c2_, &s0s1s2[1], RTMP_C2_LEN);
  return true;
}

uint8_t *RtmpHandshakeC::create_c2() {
  return c2_;
}

}
