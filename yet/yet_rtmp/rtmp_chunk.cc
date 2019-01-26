#include "rtmp_chunk.h"
#include "yet_rtmp/rtmp.hpp"
#include "yet_rtmp/rtmp_amf_op.h"
#include "chef_base/chef_stuff_op.hpp"

namespace yet {

BufferPtr RtmpChunk::msg2chunks(BufferPtr msg, const RtmpHeader &rtmp_header, std::size_t chunk_size, std::size_t &rtmp_header_len) {
  YET_LOG_ASSERT(rtmp_header.csid <= 65599, "Invalid csid when serialize chunk. {}", rtmp_header.csid);

  BufferPtr ret;

  std::size_t total = msg->readable_size();

  std::size_t suffix_chunk_len = chunk_size;
  std::size_t num_of_chunk = total / chunk_size;
  if (total % chunk_size != 0) {
    num_of_chunk++;
    suffix_chunk_len = total % chunk_size;
  }

  std::size_t max_needed_len = (chunk_size + RTMP_MAX_HEADER_LEN) * num_of_chunk;
  ret = std::make_shared<Buffer>(max_needed_len, max_needed_len);

  uint8_t header[RTMP_MAX_HEADER_LEN];
  uint8_t *p = header;
  std::size_t fmt = 0;
  uint32_t timestamp = rtmp_header.timestamp;

  if (has_prev_) {
    if (rtmp_header.msg_stream_id == prev_rtmp_header_.msg_stream_id) {
      fmt++;
      if (rtmp_header.msg_len == prev_rtmp_header_.msg_len && rtmp_header.msg_type_id == prev_rtmp_header_.msg_type_id) {
        fmt++;
        if (rtmp_header.timestamp == prev_rtmp_header_.timestamp) {
          fmt++;
        }
      }
      timestamp = rtmp_header.timestamp - prev_rtmp_header_.timestamp;
    }
  }

  *p = (fmt << 6);

  if (rtmp_header.csid >= 2 && rtmp_header.csid <= 63) {
    *p++ |= rtmp_header.csid;
  } else if (rtmp_header.csid >= 64 && rtmp_header.csid <= 319) {
    ++p;
    *p++ |= (rtmp_header.csid - 64);
  } else {
    ++p;
    *p++ |= 1;
    *p++ = (rtmp_header.csid - 64);
    *p++ = ((rtmp_header.csid - 64) >> 8);
  }

  if (fmt <= 2) {
    p = AmfOp::encode_int24(p, std::min(timestamp, (uint32_t)RTMP_MAX_TIMESTAMP_IN_MSG_HEADER));
    if (fmt <= 1) {
      p = AmfOp::encode_int24(p, rtmp_header.msg_len);
      *p++ = rtmp_header.msg_type_id;
    }
    if (fmt == 0) {
      p = AmfOp::encode_int32_le(p, rtmp_header.msg_stream_id);
    }
  }

  if (timestamp > RTMP_MAX_TIMESTAMP_IN_MSG_HEADER) {
    p = AmfOp::encode_int32(p, timestamp);
  }

  rtmp_header_len = p-header;
  uint8_t *pos = msg->read_pos();
  for (std::size_t i = 0; i < num_of_chunk; i++) {
    ret->append(header, rtmp_header_len);
    ret->append(pos + i*chunk_size, (i == num_of_chunk - 1) ? suffix_chunk_len : chunk_size);
  }

  return ret;
}

}
