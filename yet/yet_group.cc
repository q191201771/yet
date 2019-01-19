#include "yet_group.h"
#include "http_flv_pull.h"
#include "http_flv_sub.h"

namespace yet {

Group::Group(const std::string &live_name)
  : live_name_(live_name)
{
}

void Group::set_http_flv_pull(HttpFlvPullPtr pull) {
  pull->set_group(shared_from_this());
  pull_ = pull;
}

HttpFlvPullPtr Group::get_in() {
  return pull_;
}

void Group::add_out(HttpFlvSubPtr sub) {
  sub->set_group(shared_from_this());
  subs_.insert(sub);
}

void Group::del_out(HttpFlvSubPtr sub) {
  subs_.erase(sub);
}

void Group::on_connected() {

}

void Group::on_data(BufferPtr buf, const std::vector<FlvTagInfo> &tis) {
  for (auto sub : subs_) {
    sub->async_send(buf, tis);
  }
}

BufferPtr Group::get_metadata() {
  return pull_ ? pull_->get_metadata() : nullptr;
}

BufferPtr Group::get_video_seq_header() {
  return pull_ ? pull_->get_video_seq_header() : nullptr;
}

}
