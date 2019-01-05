#include "rtmp_handshake.h"
#include "rtmp_amf_op.h"
#include <chef_base/chef_stuff_op.hpp>

namespace yet {

char *RTMPHandshake::create_s0s1() {
  s0s1[0] = RTMP_VERSION;
  int32_t now_sec = static_cast<int32_t>(chef::stuff_op::unix_timestamp_msec() / 1000);
  AmfOp::encode_int32(s0s1+1, now_sec);
  // TODO random

  return s0s1;
}

char *RTMPHandshake::create_s2() {
  return s2;
}

}
