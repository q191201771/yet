/**
 * @file   yet_rtmp_chunk_op.h
 * @author pengrl
 * @date   20190127
 *
 */

#pragma once

#include "yet_rtmp/yet_rtmp.hpp"

namespace yet {

struct RtmpHeader;

class RtmpChunkOp {
  public:
    static BufferPtr msg2chunks(BufferPtr msg, const RtmpHeader &rtmp_header, const RtmpHeader *prev, std::size_t chunk_size);

    // deserialize
  private:
    RtmpChunkOp() = delete;
    RtmpChunkOp(const RtmpChunkOp &) = delete;
    const RtmpChunkOp &operator=(const RtmpChunkOp &) = delete;
};

}
