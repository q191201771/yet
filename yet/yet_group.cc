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
  pub->set_rtmp_data_cb(std::bind(&Group::on_rtmp_data, this, _1, _2, _3));
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

void Group::on_rtmp_data(RtmpSessionPtr pub, BufferPtr msg, const RtmpHeader &h) {
  RtmpHeader *prev = h.msg_type_id == RTMP_MSG_TYPE_ID_AUDIO ? prev_audio_header_ : prev_video_header_;
  //BufferPtr avc_header_chunks;
  //BufferPtr aac_header_chunks;
  BufferPtr delta_chunks;
  BufferPtr abs_chunks;

  for (auto sub : rtmp_subs_) {
    if (!sub->has_sent_video()) {
      if (avc_header_) {
        //if (!avc_header_chunks) {
        //  avc_header_chunks = RtmpChunkOp::msg2chunks(avc_header_, h, nullptr, RTMP_LOCAL_CHUNK_SIZE);
        //}

        //YET_LOG_DEBUG("send avc header.");
        sub->async_send(avc_header_);

        sub->set_has_sent_video(true);
      }

    }

    if (!sub->has_sent_audio()) {
      if (aac_header_) {
        //if (!aac_header_chunks) {
        //  aac_header_chunks = RtmpChunkOp::msg2chunks(aac_header_, h, nullptr, RTMP_LOCAL_CHUNK_SIZE);
        //}

        //YET_LOG_DEBUG("send aac header.");
        sub->async_send(aac_header_);

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
    cache_aac_header(msg, h);
    if (!prev_audio_header_) { prev_audio_header_ = new RtmpHeader(); }

    *prev_audio_header_ = h;
  } else {
    cache_avc_header(msg, h);
    if (!prev_video_header_) { prev_video_header_ = new RtmpHeader(); }

    *prev_video_header_ = h;
  }


}

void Group::cache_aac_header(BufferPtr msg, const RtmpHeader &h) {
  uint8_t *p = msg->read_pos();
  if (h.msg_type_id == RTMP_MSG_TYPE_ID_AUDIO && msg->readable_size() > 2 && (p[0] >> 4) == 0xa && p[1] == 0) {
    YET_LOG_DEBUG("Cache aac header.");
    //aac_header_ = std::make_shared<Buffer>(*msg);
    aac_header_ = RtmpChunkOp::msg2chunks(msg, h, nullptr, RTMP_LOCAL_CHUNK_SIZE);
  }
}

void Group::cache_avc_header(BufferPtr msg, const RtmpHeader &h) {
  uint8_t *p = msg->read_pos();
  if (h.msg_type_id == RTMP_MSG_TYPE_ID_VIDEO && msg->readable_size() > 2 && p[0] == 0x17 && p[1] == 0x0) {
    YET_LOG_DEBUG("Cache avc header.");
    //avc_header_ = std::make_shared<Buffer>(*msg);
    avc_header_ = RtmpChunkOp::msg2chunks(msg, h, nullptr, RTMP_LOCAL_CHUNK_SIZE);
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
