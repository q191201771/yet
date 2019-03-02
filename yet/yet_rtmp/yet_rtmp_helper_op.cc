#include "yet_rtmp_helper_op.h"
#include <stdlib.h>

namespace yet {

bool RtmpHelperOp::resolve_rtmp_url(const std::string &url, RtmpUrlStuff &out) {
  if (url.empty()) { return false; }

  std::size_t pos;
  std::string part;
  out.origin_url = url;

  pos = url.rfind("/");
  if (pos == std::string::npos) { return false; }

  out.tcurl = url.substr(0, pos);
  out.swfurl = out.tcurl;
  out.live_name_with_full_param = url.substr(pos+1);
  pos = out.live_name_with_full_param.find("?");
  if (pos == std::string::npos) {
    out.live_name = out.live_name_with_full_param;
  } else {
    out.live_name = out.live_name_with_full_param.substr(0, pos);
  }

  part = out.tcurl;
  pos = part.find("://");
  if (pos == std::string::npos) { return false; }

  out.schema = part.substr(0, pos);
  part = part.substr(pos+3);

  pos = part.find("/");
  if (pos == std::string::npos) { return false; }

  out.host = part.substr(0, pos);
  out.app_name = part.substr(pos+1);

  pos = out.host.find(":");
  if (pos == std::string::npos) {
    out.port = 1935;
  } else {
    out.port = atoi(out.host.substr(pos+1).c_str());
    out.host = out.host.substr(0, pos);
  }

  return true;
}

}
