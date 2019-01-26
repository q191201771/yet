#include "yet_group.h"
#include "yet.hpp"
#include "http_flv_pull.h"
#include "http_flv_sub.h"
#include "rtmp_session.h"

namespace yet {

Group::Group(const std::string &live_name)
  : live_name_(live_name)
{
}

void Group::dispose() {
  if (http_flv_pull_) { http_flv_pull_->dispose(); }

  if (rtmp_pub_) { rtmp_pub_->dispose(); }

  for (auto &it : http_flv_subs_) { it->dispose(); }

  http_flv_subs_.clear();

  for (auto &it : rtmp_subs_) { it->dispose(); }

  http_flv_subs_.clear();
}

void Group::set_http_flv_pull(HttpFlvPullPtr pull) {
  pull->set_group(shared_from_this());
  http_flv_pull_ = pull;
}

void Group::set_rtmp_pub(RtmpSessionPtr pub) {
  pub->set_group(shared_from_this());
  rtmp_pub_ = pub;
}

void Group::reset_rtmp_pub() {
  rtmp_pub_.reset();
}

HttpFlvPullPtr Group::get_http_flv_pull() {
  return http_flv_pull_;
}

void Group::add_http_flv_sub(HttpFlvSubPtr sub) {
  sub->set_group(shared_from_this());
  http_flv_subs_.insert(sub);
}

void Group::del_http_flv_sub(HttpFlvSubPtr sub) {
  http_flv_subs_.erase(sub);
}

void Group::add_rtmp_sub(RtmpSessionPtr sub) {
  sub->set_group(shared_from_this());
  rtmp_subs_.insert(sub);
}

void Group::del_rtmp_sub(RtmpSessionPtr sub) {
  rtmp_subs_.erase(sub);
}


void Group::on_http_flv_pull_connected() {

}

void Group::on_http_flv_data(BufferPtr buf, const std::vector<FlvTagInfo> &tis) {
  for (auto sub : http_flv_subs_) {
    sub->async_send(buf, tis);
  }
}

BufferPtr Group::get_metadata() {
  return http_flv_pull_ ? http_flv_pull_->get_metadata() : nullptr;
}

BufferPtr Group::get_video_seq_header() {
  return http_flv_pull_ ? http_flv_pull_->get_video_seq_header() : nullptr;
}

void Group::on_rtmp_data(BufferPtr msg, const RtmpHeader &h) {
  for (auto sub : rtmp_subs_) {
    if (!sub->has_sent_avc_header()) {
      if (avc_header_) { sub->async_send(avc_header_); }

      sub->set_has_sent_avc_header(true);
    }

    if (!sub->has_sent_key_frame()) {
      if (h.msg_type_id == RTMP_MSG_TYPE_ID_AUDIO) {
        //continue;
      } else if (h.msg_type_id == RTMP_MSG_TYPE_ID_VIDEO) {
        if (msg->readable_size() > 1 && msg->read_pos()[0] == 0x17) {
          sub->set_has_sent_key_frame(true);
        } else {
          continue;
        }
      }
    }

    auto chunks = rtmp_chunk_.msg2chunks(msg, h, RTMP_LOCAL_CHUNK_SIZE, false);
    sub->async_send(chunks);
  }

  cache_avc_header(msg, h);
}

void Group::cache_avc_header(BufferPtr msg, const RtmpHeader &h) {
  uint8_t *p = msg->read_pos();
  if (h.msg_type_id == RTMP_MSG_TYPE_ID_VIDEO && msg->readable_size() > 2 && p[0] == 0x17 && p[1] == 0x0) {
    YET_LOG_DEBUG("Cache avc header.");
    avc_header_ = rtmp_chunk_.msg2chunks(msg, h, RTMP_LOCAL_CHUNK_SIZE, false);
  }
}

}
