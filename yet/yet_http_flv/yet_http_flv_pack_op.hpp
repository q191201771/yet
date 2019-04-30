/**
 * @file   yet_http_flv_pack_op.hpp
 * @author pengrl
 *
 */

#pragma once

#include "chef_base/chef_be_le_op.hpp"
#include "chef_base/chef_buffer.hpp"
#include "yet_http_flv/yet_http_flv.hpp"
#include "yet_common/span.hpp"
#include <assert.h>

namespace yet {

class HttpFlvPackOp {
  public:
    static void pack_tag_header(uint8_t *buf, uint8_t type, uint32_t data_size, uint32_t timestamp) {
      assert(buf &&
             (type == FLV_TAG_HEADER_TYPE_AUDIO || type == FLV_TAG_HEADER_TYPE_VIDEO || type == FLV_TAG_HEADER_TYPE_SCRIPT_DATA) &&
             data_size <= 0xFFFFFF);

      *buf++ = type;
      buf = chef::be_le_op::write_be_ui24(buf, data_size);
      buf = chef::be_le_op::write_be_ui24(buf, timestamp & 0xffffff);
      *buf++ = timestamp >> 24;
      buf = chef::be_le_op::write_be_ui24(buf, 0);
    }

    static void write_tag_timestamp(uint8_t *buf, uint32_t timestamp) {
      buf = chef::be_le_op::write_be_ui24(buf+4, timestamp &0xffffff);
      *buf = timestamp >> 24;
    }

    static BufferPtr pack_tag(nonstd::span<const uint8_t> data, uint8_t type, uint32_t timestamp) {
      BufferPtr buf = std::make_shared<Buffer>(FLV_TAG_HEADER_LEN + data.size() + FLV_PREV_TAG_LEN);
      uint8_t *p = buf->read_pos();
      pack_tag_header(p, type, data.size(), timestamp);
      memcpy(p + FLV_TAG_HEADER_LEN, &data[0], data.size());
      chef::be_le_op::write_be_ui32(p + FLV_TAG_HEADER_LEN + data.size(), FLV_TAG_HEADER_LEN + data.size());
      buf->seek_write_pos(FLV_TAG_HEADER_LEN + data.size() + FLV_PREV_TAG_LEN);
      return buf;
    }
};

}
