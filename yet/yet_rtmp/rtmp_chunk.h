/**
 * @file   rtmp_chunk.h
 * @author pengrl
 *
 */

#pragma once

#include "yet.hpp"

namespace yet {

struct RtmpHeader {
  uint32_t csid;
  uint32_t timestamp;
  uint32_t msg_len;
  uint32_t msg_type_id;
  uint32_t msg_stream_id;

  //bool is_audio() { return msg_type_id == MSG_TYPE_ID_AUDIO; }
};

class RtmpChunk {
  public:
    RtmpChunk() {}

    BufferPtr msg2chunks(BufferPtr msg, const RtmpHeader &rtmp_header, std::size_t chunk_size, std::size_t &rtmp_header_len /*out*/);

    // deserialize
  private:
    RtmpChunk(const RtmpChunk &) = delete;
    const RtmpChunk &operator=(const RtmpChunk &) = delete;

  private:
    bool has_prev_ = false;
    RtmpHeader prev_rtmp_header_;
};

}
