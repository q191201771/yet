#include "yet_group.h"
#include "yet_http_flv/yet_http_flv_pack_op.hpp"
#include "yet_rtmp/yet_rtmp_pack_op.h"
#include "yet.hpp"
#include "yet_config.h"
#include "yet_http_flv_session_sub.h"
#include "yet_rtmp_session_pub_sub.h"
#include "yet_rtmp_session_push_pull.h"

namespace yet {

Group::Group(asio::io_context &io_ctx, const std::string &app_name, const std::string &stream_name)
  : io_ctx_(io_ctx)
  , app_name_(app_name)
  , stream_name_(stream_name)
{
  YET_LOG_INFO("[{}] [lifecycle] new Group. app_name:{}, stream_name:{}", (void *)this, app_name_, stream_name_);
}

Group::~Group() {
  YET_LOG_INFO("[{}] [lifecycle] delete Group.", (void *)this);
  if (prev_audio_header_) { delete prev_audio_header_; }
  if (prev_video_header_) { delete prev_video_header_; }
}

void Group::dispose() {
  if (rtmp_pub_) { rtmp_pub_->dispose(); }

  if (rtmp_pull_) { rtmp_pull_->dispose(); }

  for (auto &it : rtmp_subs_) { it->dispose(); }
  http_flv_subs_.clear();

  for (auto &it : http_flv_subs_) { it->dispose(); }
  http_flv_subs_.clear();

  if (rtmp_push_) { rtmp_push_->dispose(); }
}

void Group::add_rtmp_sub(RtmpSessionPubSubPtr sub) {
  rtmp_subs_.insert(sub);
  pull_rtmp_if_needed();
}

void Group::add_http_flv_sub(HttpFlvSubPtr sub) {
  sub->set_close_cb(std::bind(&Group::on_http_flv_close, this, _1));
  http_flv_subs_.insert(sub);

  pull_rtmp_if_needed();
}

bool Group::empty_totally() {
  return !rtmp_pub_ && rtmp_subs_.empty() && !rtmp_push_ && !rtmp_pull_ && http_flv_subs_.empty();
}

void Group::on_http_flv_close(HttpFlvSubPtr sub) {
  http_flv_subs_.erase(sub);
}

void Group::on_rtmp_meta_data(RtmpSessionBasePtr pub, BufferPtr msg, uint8_t *meta_pos, size_t meta_size, AmfObjectItemMapPtr meta) {
  (void)pub; (void)msg; (void)meta;

  RtmpHeader h;
  h.csid = RTMP_CSID_AMF;
  h.timestamp = 0;
  h.msg_len = meta_size;
  h.msg_type_id = RTMP_MSG_TYPE_ID_DATA_MESSAGE_AMF0;
  h.msg_stream_id = RTMP_MSID;
  rtmp_chunked_metadata_ = RtmpChunkOp::msg2chunks(meta_pos, meta_size, h, nullptr, RTMP_LOCAL_CHUNK_SIZE);
  YET_LOG_DEBUG("cache rtmp meta data.");

  for (auto &sub : rtmp_subs_) {
    sub->async_send(rtmp_chunked_metadata_);
  }

  if (rtmp_push_ && rtmp_push_->is_ready()) {
    rtmp_push_->async_send(rtmp_chunked_metadata_);
  }

  http_flv_metadata_ = HttpFlvPackOp::pack_tag(meta_pos, meta_size, FLV_TAG_HEADER_TYPE_SCRIPT_DATA, 0);
  YET_LOG_DEBUG("cache http flv meta data.");

  for (auto &sub : http_flv_subs_) {
    sub->async_send(http_flv_metadata_);
    sub->set_has_sent_metadata(true);
  }
}

void Group::send_av_data(RtmpSessionBasePtr out, BufferPtr msg, const RtmpHeader &h, BufferPtr delta_chunks, BufferPtr abs_chunks) {
  if (!out->has_sent_metadata()) {
    if (rtmp_chunked_metadata_) {
      YET_LOG_DEBUG("sent cached metadata.");
      out->async_send(rtmp_chunked_metadata_);
      out->set_has_sent_metadata(true);
    }
  }
  if (!out->has_sent_video()) {
    if (rtmp_chunked_avc_header_) {
      YET_LOG_DEBUG("sent cached avc header.");
      out->async_send(rtmp_chunked_avc_header_);
      out->set_has_sent_video(true);
    }
  }
  if (!out->has_sent_audio()) {
    if (rtmp_chunked_aac_header_) {
      YET_LOG_DEBUG("sent cached aac header.");
      out->async_send(rtmp_chunked_aac_header_);
      out->set_has_sent_audio(true);
    }
  }

  if (!out->has_sent_key_frame()) {
    if (h.msg_type_id == RTMP_MSG_TYPE_ID_AUDIO) {
      //YET_LOG_DEBUG("audio waiting key frame.");
      return;
    } else if (h.msg_type_id == RTMP_MSG_TYPE_ID_VIDEO) {
      if (msg->readable_size() > 1 && msg->read_pos()[0] == 0x17) {
        out->set_has_sent_key_frame(true);
      } else {
        //YET_LOG_DEBUG("video waiting key frame.");
        return;
      }
    }
  }

  RtmpHeader *prev = h.msg_type_id == RTMP_MSG_TYPE_ID_AUDIO ? prev_audio_header_ : prev_video_header_;

  if ((h.msg_type_id == RTMP_MSG_TYPE_ID_AUDIO)) {
    if (out->has_sent_audio()) {
      if (!delta_chunks) {
        delta_chunks = RtmpChunkOp::msg2chunks(msg, h, prev, RTMP_LOCAL_CHUNK_SIZE);
      }
      //YET_LOG_DEBUG("send audio delta.");
      out->async_send(delta_chunks);
    } else {
      if (!abs_chunks) {
        abs_chunks = RtmpChunkOp::msg2chunks(msg, h, nullptr, RTMP_LOCAL_CHUNK_SIZE);
      }
      out->async_send(abs_chunks);

      //YET_LOG_DEBUG("send audio abs.");
      out->set_has_sent_audio(true);
    }
  } else {
    if (out->has_sent_video()) {
      if (!delta_chunks) {
        delta_chunks = RtmpChunkOp::msg2chunks(msg, h, prev, RTMP_LOCAL_CHUNK_SIZE);
      }
      //YET_LOG_DEBUG("send video delta.");
      out->async_send(delta_chunks);
    } else {
      if (!abs_chunks) {
        abs_chunks = RtmpChunkOp::msg2chunks(msg, h, nullptr, RTMP_LOCAL_CHUNK_SIZE);
      }
      //YET_LOG_DEBUG("send video abs.");
      out->async_send(abs_chunks);

      out->set_has_sent_video(true);
    }
  }
}

void Group::on_rtmp_av_data(RtmpSessionBasePtr pub, BufferPtr msg, const RtmpHeader &h) {
  (void)pub;
  BufferPtr delta_chunks;
  BufferPtr abs_chunks;

  for (auto &sub : rtmp_subs_) {
    send_av_data(sub, msg, h, delta_chunks, abs_chunks);
  } // rtmp subs loop

  if (rtmp_push_ && rtmp_push_->is_ready()) {
    send_av_data(rtmp_push_, msg, h, delta_chunks, abs_chunks);
  }

  // CHEFTODO dup code with rtmp subs loop
  for (auto &sub : http_flv_subs_) {
    if (!sub->has_sent_metadata()) {
      if (http_flv_metadata_) {
        YET_LOG_DEBUG("sent cached metadata.");
        sub->async_send(http_flv_metadata_);
        sub->set_has_sent_metadata(true);
      }
    }
    if (!sub->has_sent_video()) {
      if (http_flv_avc_header_) {
        YET_LOG_DEBUG("sent cached avc header.");
        HttpFlvPackOp::write_tag_timestamp(http_flv_avc_header_->read_pos(), h.timestamp);
        sub->async_send(http_flv_avc_header_);
        sub->set_has_sent_video(true);
      }
    }
    if (!sub->has_sent_audio()) {
      if (http_flv_aac_header_) {
        YET_LOG_DEBUG("sent cached aac header.");
        HttpFlvPackOp::write_tag_timestamp(http_flv_aac_header_->read_pos(), h.timestamp);
        sub->async_send(http_flv_aac_header_);
        sub->set_has_sent_audio(true);
      }
    }

    if (!sub->has_sent_key_frame()) {
      if (h.msg_type_id == RTMP_MSG_TYPE_ID_AUDIO) {
        continue;
      } else if (h.msg_type_id == RTMP_MSG_TYPE_ID_VIDEO) {
        if (msg->readable_size() > 1 && msg->read_pos()[0] == 0x17) {
          sub->set_has_sent_key_frame(true);
        } else {
          continue;
        }
      }
    }

    if ((h.msg_type_id == RTMP_MSG_TYPE_ID_AUDIO)) {
      auto buf = HttpFlvPackOp::pack_tag(msg->read_pos(), msg->readable_size(), FLV_TAG_HEADER_TYPE_AUDIO, h.timestamp);
      sub->async_send(buf);
      sub->set_has_sent_audio(true);
    } else {
      auto buf = HttpFlvPackOp::pack_tag(msg->read_pos(), msg->readable_size(), FLV_TAG_HEADER_TYPE_VIDEO, h.timestamp);
      sub->async_send(buf);
      sub->set_has_sent_video(true);
    }

  } // http flv subs loop

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
  uint8_t *rp = msg->read_pos();
  if (h.msg_type_id == RTMP_MSG_TYPE_ID_AUDIO && msg->readable_size() > 2 && (rp[0] >> 4) == 0xa && rp[1] == 0) {
    //YET_LOG_DEBUG("cache aac header.");
    rtmp_chunked_aac_header_ = RtmpChunkOp::msg2chunks(msg, h, nullptr, RTMP_LOCAL_CHUNK_SIZE);

    http_flv_aac_header_ = HttpFlvPackOp::pack_tag(rp, msg->readable_size(), FLV_TAG_HEADER_TYPE_AUDIO, 0);
  }
}

void Group::cache_avc_header(BufferPtr msg, const RtmpHeader &h) {
  uint8_t *rp = msg->read_pos();
  if (h.msg_type_id == RTMP_MSG_TYPE_ID_VIDEO && msg->readable_size() > 2 && rp[0] == 0x17 && rp[1] == 0x0) {
    //YET_LOG_DEBUG("cache avc header.");
    rtmp_chunked_avc_header_ = RtmpChunkOp::msg2chunks(msg, h, nullptr, RTMP_LOCAL_CHUNK_SIZE);

    http_flv_avc_header_ = HttpFlvPackOp::pack_tag(rp, msg->readable_size(), FLV_TAG_HEADER_TYPE_VIDEO, 0);
  }
}

void Group::on_rtmp_pub_start(RtmpSessionPubSubPtr pub) {
  pub->set_rtmp_av_data_cb(std::bind(&Group::on_rtmp_av_data, this, _1, _2, _3));
  pub->set_rtmp_meta_data_cb(std::bind(&Group::on_rtmp_meta_data, this, _1, _2, _3, _4, _5));
  rtmp_pub_ = pub;

  auto len = RtmpPackOp::encode_user_control_stream_begin_reserve();
  auto buf = std::make_shared<Buffer>(len);
  RtmpPackOp::encode_user_control_stream_begin(buf->write_pos());
  buf->seek_write_pos(len);
  for (auto sub : rtmp_subs_) {
    sub->async_send(buf);
  }

  // CHEFTODO should we push if pull???
  if (Config::instance()->push_rtmp_if_pub() && !rtmp_push_ && rtmp_pub_) {
    rtmp_push_ = RtmpSessionPushPull::create_push(io_ctx_);
    rtmp_push_->async_start(Config::instance()->rtmp_push_host(), Config::instance()->rtmp_push_port(), rtmp_pub_->app_name(),
                            rtmp_pub_->stream_name());
    rtmp_push_->set_rtmp_session_close_cb(std::bind(&Group::on_rtmp_session_close, this, _1)); // for pub & sub & push & pull session
  }
}

void Group::on_rtmp_pub_stop() {
  auto len = RtmpPackOp::encode_user_control_stream_eof_reserve();
  auto buf = std::make_shared<Buffer>(len);
  RtmpPackOp::encode_user_control_stream_eof(buf->write_pos());
  buf->seek_write_pos(len);
  for (auto sub : rtmp_subs_) {
    sub->async_send(buf);
  }
}

void Group::on_rtmp_session_close(RtmpSessionBasePtr session) {
  // CHEFTODO re-push re-pull?
  switch (session->type()) {
  case RtmpSessionType::PUB:  rtmp_pub_.reset(); break;
  case RtmpSessionType::SUB:  rtmp_subs_.erase(session->cast_to_pub_sub()); break;
  case RtmpSessionType::PULL: rtmp_pull_.reset(); break;
  case RtmpSessionType::PUSH: rtmp_push_.reset(); break;
  default:                    YET_LOG_ASSERT(0, "invalid.");
  }
}

bool Group::has_in() {
  return rtmp_pub_ || rtmp_pull_;
}

void Group::pull_rtmp_if_needed() {
  if (Config::instance()->pull_rtmp_if_stream_not_exist() && !has_in()) {
    rtmp_pull_ = RtmpSessionPushPull::create_pull(io_ctx_);
    rtmp_pull_->async_start(Config::instance()->rtmp_pull_host(), Config::instance()->rtmp_pull_port(), app_name_, stream_name_);
    rtmp_pull_->set_rtmp_session_close_cb(std::bind(&Group::on_rtmp_session_close, this, _1));
    rtmp_pull_->set_rtmp_meta_data_cb(std::bind(&Group::on_rtmp_meta_data, this, _1, _2, _3, _4, _5));
    rtmp_pull_->set_rtmp_av_data_cb(std::bind(&Group::on_rtmp_av_data, this, _1, _2, _3));
  }
}

}
