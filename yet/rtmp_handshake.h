/**
 * @file   rtmp_handshake.h
 * @author pengrl
 *
 */

#pragma once

#include "yet_inner.hpp"

namespace yet {

class RTMPHandshake {
  public:
    char *create_s0s1();
    char *create_s2();

  private:
    char s0s1[S0S1_LEN];
    char s2[S2_LEN];
};

}
