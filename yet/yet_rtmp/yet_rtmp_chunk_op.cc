#include "yet_rtmp_chunk_op.h"
#include "yet_rtmp/yet_rtmp.hpp"
#include "yet_rtmp/yet_rtmp_amf_op.h"
#include "chef_base/chef_stuff_op.hpp"

namespace yet {

BufferPtr RtmpChunkOp::msg2chunks(BufferPtr msg, const RtmpHeader &rtmp_header, const RtmpHeader *prev, std::size_t chunk_size) {
  return msg2chunks(msg->read_pos(), msg->readable_size(), rtmp_header, prev, chunk_size);
}

BufferPtr RtmpChunkOp::msg2chunks(uint8_t *msg, std::size_t msg_size, const RtmpHeader &rtmp_header, const RtmpHeader *prev, std::size_t chunk_size) {
  YET_LOG_ASSERT(rtmp_header.csid <= 65599, "Invalid csid when serialize chunk. {}", rtmp_header.csid);

  BufferPtr ret;

  std::size_t suffix_chunk_len = chunk_size;
  std::size_t num_of_chunk = msg_size / chunk_size;
  if (msg_size % chunk_size != 0) {
    num_of_chunk++;
    suffix_chunk_len = msg_size % chunk_size;
  }

  std::size_t max_needed_len = (chunk_size + RTMP_MAX_HEADER_LEN) * num_of_chunk;
  ret = std::make_shared<Buffer>(max_needed_len, max_needed_len);

  uint8_t header[RTMP_MAX_HEADER_LEN];
  uint8_t *p = header;
  std::size_t fmt = 0;

  uint32_t timestamp;
  if (!prev) {
    timestamp = rtmp_header.timestamp;
  } else {
    if (rtmp_header.msg_stream_id == prev->msg_stream_id) {
      fmt++;
      if (rtmp_header.msg_len == prev->msg_len && rtmp_header.msg_type_id == prev->msg_type_id) {
        fmt++;
        if (rtmp_header.timestamp == prev->timestamp) {
          fmt++;
        }
      }
      timestamp = rtmp_header.timestamp - prev->timestamp;
    } else {
      timestamp = rtmp_header.timestamp;
      YET_LOG_ASSERT(0, "{} {} {} {} {}", rtmp_header.msg_stream_id, prev->msg_stream_id, rtmp_header.timestamp, prev->timestamp, timestamp);
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

  std::size_t rtmp_header_len = p-header;
  for (std::size_t i = 0; i < num_of_chunk; i++) {
    ret->append(header, rtmp_header_len);
    ret->append(msg + i*chunk_size, (i == num_of_chunk - 1) ? suffix_chunk_len : chunk_size);
  }

  return ret;

}

}
