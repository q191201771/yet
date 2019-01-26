#include "rtmp_handshake.h"
#include "yet_rtmp/hmac_sha256_adapter.hpp"
#include "yet_rtmp/rtmp_amf_op.h"
#include "chef_base/chef_stuff_op.hpp"

namespace yet {

// TODO refactor me after use HS as client

// old style: obs rtmpdump
// new style: ffmpeg

static constexpr std::size_t RTMP_HANDSHAKE_KEYLEN = 32;

static constexpr uint8_t RTMP_SERVER_VERSION[] = {
    0x0D, 0x0E, 0x0A, 0x0D
};

static constexpr uint8_t RTMP_CLIENT_VERSION[] = {
    0x0C, 0x00, 0x0D, 0x0E
};

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

static constexpr std::size_t RTMP_SERVER_FULL_KEYLEN = sizeof(RTMP_SERVER_KEY);
static constexpr std::size_t RTMP_SERVER_PART_KEYLEN = 36;
static constexpr std::size_t RTMP_CLIENT_FULL_KEYLEN = sizeof(RTMP_CLIENT_KEY);
static constexpr std::size_t RTMP_CLIENT_PART_KEYLEN = 30;

static bool rtmp_make_digest(const uint8_t *buf, std::size_t buf_len, const uint8_t *skip,
                             const uint8_t *key, std::size_t key_len,
                             uint8_t *dst)
{
  //YET_LOG_DEBUG("SHA256 key:{} len:{}", chef::stuff_op::bytes_to_hex(key, key_len, 16), key_len);

  HMACSHA256 crypto;
  crypto.init(key, key_len);

  if (skip) {
    // left
    if (skip != buf) {
      //YET_LOG_INFO("SHA256 left:{} len:{}", chef::stuff_op::bytes_to_hex(buf, skip-buf, 16), skip-buf);
      //HMAC_Update(hmac, buf, skip-buf);
      crypto.update(buf, skip-buf);
    }
    // right
    std::size_t right_len = buf + buf_len - skip - RTMP_HANDSHAKE_KEYLEN;
    if (right_len > 0) {
      //YET_LOG_INFO("SHA256 right:{} len:{}", chef::stuff_op::bytes_to_hex(skip+RTMP_HANDSHAKE_KEYLEN, right_len, 16), right_len);
      //HMAC_Update(hmac, skip + RTMP_HANDSHAKE_KEYLEN, right_len);
      //uint8_t digest2[32];
      crypto.update(skip + RTMP_HANDSHAKE_KEYLEN, right_len);
    }
    //YET_LOG_INFO("SHA256 dst:{} len:{}", chef::stuff_op::bytes_to_hex(dst, 32, 16), 32);

  } else {
    //YET_LOG_INFO("SHA256 full:{} len:{}", chef::stuff_op::bytes_to_hex(buf, buf_len, 16), buf_len);
    //HMAC_Update(hmac, buf, buf_len);
    crypto.update(buf, buf_len);
    //uint8_t digest2[32];
    //cryptlite::hmac<cryptlite::sha256>::calc(buf, buf_len, key, key_len, dst);
    //YET_LOG_INFO("SHA256 dst:{} len:{}", chef::stuff_op::bytes_to_hex(dst, 32, 16), 32);
  }

  //HMAC_Final(hmac, dst, &len);
  //std::size_t dst_len;
  crypto.final(dst);
  //YET_LOG_INFO("SHA256 dst:{}", chef::stuff_op::bytes_to_hex(dst, 32, 16));
  return true;
}

static int rtmp_find_digest(const uint8_t *buf, std::size_t buf_len, std::size_t base, const uint8_t *key, std::size_t key_len) {
  std::size_t offs = buf[base] + buf[base+1] + buf[base+2] + buf[base+3];
  offs = (offs % 728) + base + 4;
  //YET_LOG_INFO("CHEFGREPME offs:{}", offs);
  const uint8_t *skip = buf + offs;

  uint8_t digest[RTMP_HANDSHAKE_KEYLEN];
  if (!rtmp_make_digest(buf, buf_len, skip, key, key_len, digest)) {
    return -1;
  }

  if (memcmp(digest, skip, RTMP_HANDSHAKE_KEYLEN) == 0) {
    //YET_LOG_INFO("CHEFGREPME bingo.");
    return offs;
  }
  return -1;
}

bool RtmpHandshake::rtmp_handshake_create_challenge(uint8_t *buf, std::size_t buf_len, const uint8_t *version,
                                                    const uint8_t *key, std::size_t key_len)
{
  *buf++ = RTMP_VERSION;
  timestamp_sent_s1_ = static_cast<int>(chef::stuff_op::unix_timestamp_msec() / 1000);
  AmfOp::encode_int32(buf, timestamp_sent_s1_);
  memcpy(buf+4, version, 4);
  // TODO random

  std::size_t offs = buf[8] + buf[9] + buf[10] + buf[11];
  offs = (offs % 728) + 12;
  uint8_t *skip = (uint8_t *)buf + offs;
  //YET_LOG_INFO("offs:{}", offs);

  if (!rtmp_make_digest((const uint8_t *)buf, buf_len-1, skip, key, key_len, skip)) {
    return false;
  }
  return true;
}

bool RtmpHandshake::rtmp_handshake_parse_challenge(const uint8_t *buf, std::size_t buf_len,
                                                   const uint8_t *peer_key, std::size_t peer_key_len,
                                                   const uint8_t *key, std::size_t key_len)
{
  if (buf[0] != RTMP_VERSION) {
    YET_LOG_ERROR("Handle c0c1 failed since version invalid. ver:{}", buf[0]);
    return false;
  }
  buf++;

  int peer_epoch;
  AmfOp::decode_int32(buf, 4, &peer_epoch, nullptr);
  int ver;
  AmfOp::decode_int32(buf+4, 4, &ver, nullptr);
  if (ver == 0) {
    YET_LOG_INFO("Rtmp handshake old style.");
    is_old_ = true;
    return true;
  }
  // key before digest
  int offs = rtmp_find_digest((const uint8_t *)buf, buf_len-1, 764+8, peer_key, peer_key_len);
  if (offs == -1) {
    // digest before key
    offs = rtmp_find_digest((const uint8_t *)buf, buf_len-1, 8, peer_key, peer_key_len);
  }
  if (offs == -1) {
    YET_LOG_INFO("Rtmp handshake old style.");
    is_old_ = true;
    return true;
  }

  YET_LOG_INFO("Rtmp handshake new style. offs:{}", offs);
  is_old_ = false;
  // TODO random

  uint8_t digest[RTMP_HANDSHAKE_KEYLEN];
  if (!rtmp_make_digest((const uint8_t *)buf+offs, RTMP_HANDSHAKE_KEYLEN, nullptr, key, key_len, digest)) {
    YET_LOG_ERROR("CHEFGREPME Make s2 digest failed.");
    return false;
  }

  static constexpr std::size_t digest_pos = RTMP_S2_LEN - RTMP_HANDSHAKE_KEYLEN;
  if (!rtmp_make_digest((const uint8_t *)s2_, RTMP_S2_LEN, (const uint8_t *)s2_+digest_pos, digest, RTMP_HANDSHAKE_KEYLEN, (uint8_t *)s2_+ digest_pos)) {
    YET_LOG_ERROR("CHEFGREPME Make s2 final digest failed.");
    return false;
  }

  return true;
}

bool RtmpHandshake::handle_c0c1(const uint8_t *c0c1, std::size_t len) {
  if (len < RTMP_C0C1_LEN) {
    YET_LOG_ERROR("Handle c0c1 failed since len too short. len:{}", len);
    return false;
  }

  //YET_LOG_DEBUG("CHEFGREPME {}", chef::stuff_op::bytes_to_hex((const uint8_t *)c0c1, 13, 32));

  rtmp_handshake_parse_challenge(c0c1, len, RTMP_CLIENT_KEY, RTMP_CLIENT_PART_KEYLEN, RTMP_SERVER_KEY, RTMP_SERVER_FULL_KEYLEN);

  if (is_old_) {
    AmfOp::decode_int32(c0c1+1, len-1, &timestamp_recvd_c1_, nullptr);
    //YET_LOG_DEBUG("timestamp_recvd_c1_:{}", timestamp_recvd_c1_);
    memcpy(s2_, c0c1+1, len-1);
  }
  return true;
}

uint8_t *RtmpHandshake::create_s0s1() {
  if (is_old_) {
    s0s1_[0] = RTMP_VERSION;
    timestamp_sent_s1_ = static_cast<int>(chef::stuff_op::unix_timestamp_msec() / 1000);
    AmfOp::encode_int32(s0s1_+1, timestamp_sent_s1_);
    memset(s0s1_+5, 0, 4);

    // TODO random 1528

    return s0s1_;
  }

  rtmp_handshake_create_challenge(s0s1_, RTMP_S0S1_LEN, RTMP_SERVER_VERSION, RTMP_SERVER_KEY, RTMP_SERVER_PART_KEYLEN);
  return s0s1_;
}

uint8_t *RtmpHandshake::create_s2() {
  if (is_old_) {
    AmfOp::encode_int32(s2_, timestamp_recvd_c1_);
    AmfOp::encode_int32(s2_+4, timestamp_sent_s1_);

    return s2_;
  }

  return s2_;
}

}
