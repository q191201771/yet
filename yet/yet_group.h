/**
 * @file   yet_group.h
 * @author pengrl
 *
 */

#pragma once

#include <string>
#include <unordered_set>
#include <vector>
#include <asio.hpp>
#include "chef_base/chef_snippet.hpp"
#include "yet_rtmp/yet_rtmp_chunk_op.h"
#include "yet.hpp"

namespace yet {

class Group : public std::enable_shared_from_this<Group> {
  public:
    explicit Group(asio::io_context &io_ctx, const std::string &app_name, const std::string &live_name);
    ~Group();

    void dispose();

    void add_rtmp_sub(RtmpSessionPubSubPtr sub);

    void add_http_flv_sub(HttpFlvSubPtr sub);

    bool empty_totally();

  public:
    void on_rtmp_pub_start(RtmpSessionPubSubPtr pub);
    void on_rtmp_pub_stop();

  public:
    void on_http_flv_close(HttpFlvSubPtr sub);

  public:
    void on_rtmp_av_data(RtmpSessionBasePtr pub, BufferPtr msg, const RtmpHeader &h);
    void on_rtmp_meta_data(RtmpSessionBasePtr pub, BufferPtr msg, uint8_t *meta_pos, size_t meta_size, AmfObjectItemMapPtr meta);
    void on_rtmp_session_close(RtmpSessionBasePtr session);

  private:
    void cache_avc_header(BufferPtr msg, const RtmpHeader &h);
    void cache_aac_header(BufferPtr msg, const RtmpHeader &h);

    void send_av_data(RtmpSessionBasePtr out, BufferPtr msg, const RtmpHeader &h, BufferPtr delta_chunks, BufferPtr abs_chunks);

    bool has_in();

    void pull_rtmp_if_needed();

  private:
    Group(const Group &) = delete;
    Group &operator=(const Group &) = delete;

  private:
    using HttpFlvSubs = std::unordered_set<HttpFlvSubPtr>;
    using RtmpSubs = std::unordered_set<RtmpSessionPubSubPtr>;

  private:
    asio::io_context       &io_ctx_;

    CHEF_PROPERTY(std::string, app_name);
    CHEF_PROPERTY(std::string, stream_name);

  private:
    RtmpSessionPubSubPtr   rtmp_pub_;
    RtmpSessionPushPullPtr rtmp_pull_;
    RtmpSubs               rtmp_subs_;
    RtmpSessionPushPullPtr rtmp_push_;
    HttpFlvSubs            http_flv_subs_;
    RtmpHeader             *prev_audio_header_=nullptr;
    RtmpHeader             *prev_video_header_=nullptr;
    BufferPtr              rtmp_chunked_metadata_;
    BufferPtr              rtmp_chunked_avc_header_;
    BufferPtr              rtmp_chunked_aac_header_;
    BufferPtr              http_flv_metadata_;
    BufferPtr              http_flv_avc_header_;
    BufferPtr              http_flv_aac_header_;
};

}
