#include "yet_group.h"
#include "http_flv_in.h"

namespace yet {

Group::Group(const std::string &live_name)
  : live_name_(live_name)
{
}

void Group::set_http_flv_in(HttpFlvInPtr hfi) {
  hfi_ = hfi;
}

void Group::add_out(HttpFlvSessionPtr hfs) {
  hfss_.push_back(hfs);
}

}
