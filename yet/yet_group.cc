#include "yet_group.h"
#include "yet.hpp"
#include "yet_rtmp/yet_rtmp_pack_op.h"
#include "yet_http_flv_pull.h"
#include "yet_http_flv_sub.h"
#include "yet_rtmp_session.h"

namespace yet {

Group::Group(const std::string &live_name)
  : live_name_(live_name)
{
  YET_LOG_DEBUG("Group() {}", (void *)this);
}

Group::~Group() {
  YET_LOG_DEBUG("~Group() {}", (void *)this);
  if (prev_audio_header_) { delete prev_audio_header_; }
  if (prev_video_header_) { delete prev_video_header_; }
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
  pub->set_rtmp_av_data_cb(std::bind(&Group::on_rtmp_av_data, this, _1, _2, _3));
  pub->set_rtmp_meta_data_cb(std::bind(&Group::on_rtmp_meta_data, this, _1, _2, _3, _4, _5));
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
  sub->set_close_cb(std::bind(&Group::on_http_flv_close, shared_from_this(), _1));
  http_flv_subs_.insert(sub);
}

void Group::del_http_flv_sub(HttpFlvSubPtr sub) {
  http_flv_subs_.erase(sub);
}

void Group::add_rtmp_sub(RtmpSessionPtr sub) {
  rtmp_subs_.insert(sub);
}

void Group::del_rtmp_sub(RtmpSessionPtr sub) {
  rtmp_subs_.erase(sub);
}


void Group::on_http_flv_pull_connected() {

}

void Group::on_http_flv_close(HttpFlvSubPtr sub) {
  http_flv_subs_.erase(sub);
}

void Group::on_http_flv_data(BufferPtr buf, const std::vector<FlvTagInfo> &tis) {
  for (auto sub : http_flv_subs_) {
    sub->async_send(buf, tis);
  }
}

BufferPtr Group::get_metadata() {
  return http_flv_pull_ ? http_flv_pull_->get_metadata() : nullptr;
}

BufferPtr Group::get_avc_header() {
  return http_flv_pull_ ? http_flv_pull_->get_avc_header() : nullptr;
}

BufferPtr Group::get_aac_header() {
  return http_flv_pull_ ? http_flv_pull_->get_aac_header() : nullptr;
}

void Group::on_rtmp_meta_data(RtmpSessionPtr pub, BufferPtr msg, uint8_t *meta_pos, std::size_t meta_size, AmfObjectItemMapPtr meta) {
  (void)msg;
  RtmpHeader h;
  h.csid = RTMP_CSID_AMF;
  h.timestamp = 0;
  h.msg_len = meta_size;
  h.msg_type_id = RTMP_MSG_TYPE_ID_DATA_MESSAGE_AMF0;
  h.msg_stream_id = RTMP_MSID;
  rtmp_metadata_ = RtmpChunkOp::msg2chunks(meta_pos, meta_size, h, nullptr, RTMP_LOCAL_CHUNK_SIZE);

  for (auto &sub : rtmp_subs_) {
    sub->async_send(rtmp_metadata_);
  }
}

void Group::on_rtmp_av_data(RtmpSessionPtr pub, BufferPtr msg, const RtmpHeader &h) {
  RtmpHeader *prev = h.msg_type_id == RTMP_MSG_TYPE_ID_AUDIO ? prev_audio_header_ : prev_video_header_;
  BufferPtr delta_chunks;
  BufferPtr abs_chunks;

  for (auto &sub : rtmp_subs_) {
    if (!sub->has_sent_metadata()) {
      if (rtmp_metadata_) {
        YET_LOG_DEBUG("sent cached metadata.");
        sub->async_send(rtmp_metadata_);
        sub->set_has_sent_metadata(true);
      }
    }

    if (!sub->has_sent_video()) {
      if (rtmp_avc_header_) {
        YET_LOG_DEBUG("sent cached avc header.");
        sub->async_send(rtmp_avc_header_);
        sub->set_has_sent_video(true);
      }

    }

    if (!sub->has_sent_audio()) {
      if (rtmp_aac_header_) {
        YET_LOG_DEBUG("sent cached aac header.");
        sub->async_send(rtmp_aac_header_);
        sub->set_has_sent_audio(true);
      }

    }

    if (!sub->has_sent_key_frame()) {
      if (h.msg_type_id == RTMP_MSG_TYPE_ID_AUDIO) {
        //YET_LOG_DEBUG("audio waiting key frame.");
        continue;
      } else if (h.msg_type_id == RTMP_MSG_TYPE_ID_VIDEO) {
        if (msg->readable_size() > 1 && msg->read_pos()[0] == 0x17) {
          sub->set_has_sent_key_frame(true);
        } else {
          //YET_LOG_DEBUG("video waiting key frame.");
          continue;
        }
      }
    }

    if ((h.msg_type_id == RTMP_MSG_TYPE_ID_AUDIO)) {
      if (sub->has_sent_audio()) {
        if (!delta_chunks) {
          delta_chunks = RtmpChunkOp::msg2chunks(msg, h, prev, RTMP_LOCAL_CHUNK_SIZE);
        }
        //YET_LOG_DEBUG("send audio delta.");
        sub->async_send(delta_chunks);
      } else {
        if (!abs_chunks) {
          abs_chunks = RtmpChunkOp::msg2chunks(msg, h, nullptr, RTMP_LOCAL_CHUNK_SIZE);
        }
        sub->async_send(abs_chunks);

        //YET_LOG_DEBUG("send audio abs.");
        sub->set_has_sent_audio(true);
      }
    } else {
      if (sub->has_sent_video()) {
        if (!delta_chunks) {
          delta_chunks = RtmpChunkOp::msg2chunks(msg, h, prev, RTMP_LOCAL_CHUNK_SIZE);
        }
        //YET_LOG_DEBUG("send video delta.");
        sub->async_send(delta_chunks);
      } else {
        if (!abs_chunks) {
          abs_chunks = RtmpChunkOp::msg2chunks(msg, h, nullptr, RTMP_LOCAL_CHUNK_SIZE);
        }
        //YET_LOG_DEBUG("send video abs.");
        sub->async_send(abs_chunks);

        sub->set_has_sent_video(true);
      }
    }

  }

  if (h.msg_type_id == RTMP_MSG_TYPE_ID_AUDIO) {
    cache_rtmp_aac_header(msg, h);
    if (!prev_audio_header_) { prev_audio_header_ = new RtmpHeader(); }

    *prev_audio_header_ = h;
  } else {
    cache_rtmp_avc_header(msg, h);
    if (!prev_video_header_) { prev_video_header_ = new RtmpHeader(); }

    *prev_video_header_ = h;
  }


}

void Group::cache_rtmp_aac_header(BufferPtr msg, const RtmpHeader &h) {
  uint8_t *p = msg->read_pos();
  if (h.msg_type_id == RTMP_MSG_TYPE_ID_AUDIO && msg->readable_size() > 2 && (p[0] >> 4) == 0xa && p[1] == 0) {
    YET_LOG_DEBUG("Cache aac header.");
    rtmp_aac_header_ = RtmpChunkOp::msg2chunks(msg, h, nullptr, RTMP_LOCAL_CHUNK_SIZE);
  }
}

void Group::cache_rtmp_avc_header(BufferPtr msg, const RtmpHeader &h) {
  uint8_t *p = msg->read_pos();
  if (h.msg_type_id == RTMP_MSG_TYPE_ID_VIDEO && msg->readable_size() > 2 && p[0] == 0x17 && p[1] == 0x0) {
    YET_LOG_DEBUG("Cache avc header.");
    rtmp_avc_header_ = RtmpChunkOp::msg2chunks(msg, h, nullptr, RTMP_LOCAL_CHUNK_SIZE);
  }
}

void Group::on_rtmp_publish() {
  std::size_t len = RtmpPackOp::encode_rtmp_msg_user_control_stream_begin_reserve();
  BufferPtr buf = std::make_shared<Buffer>(len);
  RtmpPackOp::encode_user_control_stream_begin(buf->write_pos());
  buf->seek_write_pos(len);
  for (auto sub : rtmp_subs_) {
    sub->async_send(buf);
  }
}

void Group::on_rtmp_publish_stop() {
  std::size_t len = RtmpPackOp::encode_rtmp_msg_user_control_stream_eof_reserve();
  BufferPtr buf = std::make_shared<Buffer>(len);
  RtmpPackOp::encode_user_control_stream_eof(buf->write_pos());
  buf->seek_write_pos(len);
  for (auto sub : rtmp_subs_) {
    sub->async_send(buf);
  }
}

void Group::on_rtmp_session_close(RtmpSessionPtr session) {
       if (session->type() == RTMP_SESSION_TYPE_PUB) { reset_rtmp_pub(); }
  else if (session->type() == RTMP_SESSION_TYPE_SUB) { del_rtmp_sub(session); }
}

}
