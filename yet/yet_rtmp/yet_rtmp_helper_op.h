/**
 * @file   yet_rtmp_helper_op.h
 * @author pengrl
 *
 */

#pragma once

#include <string>
#include <cinttypes>

namespace yet {

struct RtmpUrlStuff {
  std::string origin_url;
  std::string tcurl;
  std::string swfurl;
  std::string schema;
  std::string host;
  uint16_t    port;
  std::string app_name;
  std::string live_name;
  std::string live_name_with_full_param;
};

class RtmpHelperOp {
  public:
    // rtmp://host:port/app_name/live_name?a=1&b=2
    // origin_url -> rtmp://host:port/app_name/live_name?a=1&b=2
    // tcurl -> rtmp://host:port/app_name
    // swfurl -> CHEFTODO now is same as tcurl
    // schema -> rtmp
    // host -> host
    // port -> port, default 1935 if not exist in <url>
    // app_name -> app_name
    // live_name -> live_name
    // live_name_with_full_param -> live_name?a=1&b=2
    static bool resolve_rtmp_url(const std::string &url, RtmpUrlStuff &out);
};

}
