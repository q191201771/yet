/**
 * @file   yet_http_flv_buffer_t.hpp
 * @author pengrl
 *
 */

#pragma once

#include <sstream>
#include "yet_http_flv/yet_http_flv.hpp"
#include "yet_common/yet_common.hpp"

namespace yet {

enum FlvTagType {
  FLVTAGTYPE_METADATA = FLV_TAG_HEADER_TYPE_SCRIPT_DATA,
  FLVTAGTYPE_VIDEO = FLV_TAG_HEADER_TYPE_VIDEO,
  FLVTAGTYPE_AUDIO = FLV_TAG_HEADER_TYPE_AUDIO
};

struct FlvTagInfo {
  enum FlvTagType  tag_type;
  uint8_t         *tag_pos;
  std::size_t      tag_whole_size; // with PreviousTagSizeN
};

static std::string stringify_stl_one_(const FlvTagInfo &ti) {
  std::ostringstream oss;
  oss << "(" << (int)ti.tag_type << "," << (void *)ti.tag_pos << "," << ti.tag_whole_size << ")";
  return oss.str();
}

/// return 0 if complete, otherwise return miss length
static std::size_t check_tag_complete(BufferPtr buf, const FlvTagInfo &ti) {
  if (std::size_t(buf->write_pos() - ti.tag_pos) >= ti.tag_whole_size) { return 0; }

  return ti.tag_whole_size - (buf->write_pos() - ti.tag_pos);
}

static std::size_t get_prefix_extra(BufferPtr buf, const std::vector<FlvTagInfo> &tis) {
  if (tis.empty()) { return buf->readable_size(); }

  return tis[0].tag_pos - buf->read_pos();
}

class TagCacheBase {
  public:
    TagCacheBase(bool keeping_update,
                 std::size_t init_buf_len,
                 std::size_t shrink_buf_len)
      : keeping_update_(keeping_update)
      , buf_(std::make_shared<Buffer>(init_buf_len, shrink_buf_len))
    {}

    virtual ~TagCacheBase() {}

    void on_buffer(BufferPtr buf, const std::vector<FlvTagInfo> &tis) {
      if (!keeping_update_ && is_cached_) { return; }

      if (miss_len_ == 0) {
        for (auto &ti : tis) {
          if (!is_target_tag(ti)) {
            continue;
          }

          buf_->clear();
          buf_->reserve(ti.tag_whole_size);
          miss_len_ = check_tag_complete(buf, ti);
          if (miss_len_ == 0) {
            buf_->append(ti.tag_pos, ti.tag_whole_size);
            is_cached_ = true;
          } else {
            buf_->append(ti.tag_pos, ti.tag_whole_size - miss_len_);
          }
        }
      } else {
        auto extra_len = get_prefix_extra(buf, tis);
        buf_->append(buf->read_pos(), extra_len);
        miss_len_ -= extra_len;
        if (miss_len_ == 0) {
          is_cached_ = true;
        }
      }
    }

    virtual bool is_target_tag(const FlvTagInfo &ti) = 0;

    BufferPtr buf() { return (is_cached_ && buf_ && buf_->readable_size() > 0) ? buf_ : nullptr; }

  private:
    const bool  keeping_update_;
    BufferPtr   buf_;
    bool        is_cached_ = false;
    std::size_t miss_len_ = 0;
};

class TagCacheMetadata : public TagCacheBase {
  public:
    TagCacheMetadata() : TagCacheBase(false, BUF_INIT_LEN_METADATA, BUF_SHRINK_LEN_METADATA) {}
    virtual ~TagCacheMetadata() {}

    bool is_target_tag(const FlvTagInfo &ti) {
      return ti.tag_type == FLVTAGTYPE_METADATA;
    }
};

class TagCacheAvcHeader : public TagCacheBase {
  public:
    TagCacheAvcHeader() : TagCacheBase(true, BUF_INIT_LEN_AVC_HEADER, BUF_SHRINK_LEN_AVC_HEADER) {}
    virtual ~TagCacheAvcHeader() {}

    bool is_target_tag(const FlvTagInfo &ti) {
      return ti.tag_type == FLVTAGTYPE_VIDEO && *(ti.tag_pos + 11) == 0x17 && *(ti.tag_pos + 12) == 0x0;
    }
};

class TagCacheAacHeader : public TagCacheBase {
  public:
    TagCacheAacHeader() : TagCacheBase(true, BUF_INIT_LEN_AAC_HEADER, BUF_SHRINK_LEN_AAC_HEADER) {}
    virtual ~TagCacheAacHeader() {}

    bool is_target_tag(const FlvTagInfo &ti) {
      return ti.tag_type == FLVTAGTYPE_AUDIO && ((*(ti.tag_pos + 11) >> 4) == 0xa) && (*(ti.tag_pos + 12) == 0x0);
    }
};

}
