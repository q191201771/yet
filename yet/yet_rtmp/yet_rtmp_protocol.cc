#include "yet_rtmp/yet_rtmp_protocol.h"
#include "yet_rtmp/yet_rtmp_amf_op.h"

namespace yet {

void RtmpProtocol::update_peer_chunk_size(std::size_t val) {
  peer_chunk_size_ = val;
}

void RtmpProtocol::try_compose(Buffer &buf, CompleteMessageCb cb) {
  for (; buf.readable_size() > 0; ) {
    auto *p = buf.read_pos();

    if (!header_done_) {
      auto readable_size = buf.readable_size();

      // 5.3.1.1. Chunk Basic Header 1,2,3bytes
      uint8_t fmt = (*p >> 6) & 0x03; // 2bits
      curr_csid_ = *p & 0x3f; // 6 bits

      std::size_t basic_header_len = 1;
      if (curr_csid_ == 0) {
        basic_header_len = 2;
        if (readable_size < 2) { return; }

        curr_csid_ = 64 + *(p+1);
      } else if (curr_csid_ == 1) {
        basic_header_len = 3;
        if (readable_size < 3) { return; }

        curr_csid_ = 64 + *(p+1) + (*(p+2) * 256);
      }

      p += basic_header_len;
      readable_size -= basic_header_len;

      auto stream = get_or_create_stream(curr_csid_);

      // 5.3.1.2. Chunk Message Header 11,7,3,0bytes
      switch (fmt) {
      case 0:
        if (readable_size < RTMP_FMT_2_MSG_HEADER_LEN[fmt]) { return; }

        p = AmfOp::decode_int24(p, 3, (int *)&stream->header.timestamp, nullptr);
        stream->timestamp_abs = stream->header.timestamp;
        p = AmfOp::decode_int24(p, 3, (int *)&stream->msg_len, nullptr);
        stream->header.msg_type_id = *p++;
        p = AmfOp::decode_int32_le(p, 4, (int *)&stream->header.msg_stream_id, nullptr);
        break;
      case 1:
        if (readable_size < RTMP_FMT_2_MSG_HEADER_LEN[fmt]) { return; }

        p = AmfOp::decode_int24(p, 3, (int *)&stream->header.timestamp, nullptr);
        stream->timestamp_abs += stream->header.timestamp;
        p = AmfOp::decode_int24(p, 3, (int *)&stream->msg_len, nullptr);
        stream->header.msg_type_id = *p++;
        break;
      case 2:
        if (readable_size < RTMP_FMT_2_MSG_HEADER_LEN[fmt]) { return; }

        p = AmfOp::decode_int24(p, 3, (int *)&stream->header.timestamp, nullptr);
        stream->timestamp_abs += stream->header.timestamp;
      case 3:
        // noop
        break;
      default:
        // since fmt parsed from 2 bits, code should never reach here
        break;
      }

      readable_size -= RTMP_FMT_2_MSG_HEADER_LEN[fmt];

      // 5.3.1.3 Extended Timestamp
      auto has_ext_ts = (stream->header.timestamp == RTMP_MAX_TIMESTAMP_IN_MSG_HEADER);
      if (has_ext_ts) {
        if (readable_size < 4) { return; }

        p = AmfOp::decode_int32(p, 4, (int *)&stream->header.timestamp, nullptr);
        switch (fmt) {
        case 0:
          stream->timestamp_abs = stream->header.timestamp;
          break;
        case 1:
        case 2:
          stream->timestamp_abs += stream->header.timestamp;
          break;
        case 3:
          // noop
          break;
        default:
          // since fmt parsed from 2 bits, code should never reach here
          break;
        }

        readable_size -= 4;
      }

      header_done_ = true;
      buf.erase(basic_header_len + RTMP_FMT_2_MSG_HEADER_LEN[fmt] + (has_ext_ts ? 4 : 0));
    } // header parsed done.

    curr_stream_ = get_or_create_stream(curr_csid_);

    std::size_t needed_size;
    if (curr_stream_->msg_len <= peer_chunk_size_) {
      needed_size = curr_stream_->msg_len;
    } else {
      auto whole_needed = curr_stream_->msg_len - curr_stream_->msg->readable_size();
      needed_size = std::min(whole_needed, peer_chunk_size_);
    }

    //YET_LOG_DEBUG("{} {} {}", needed_size, buf.readable_size(), curr_stream_->msg_len);

    if (buf.readable_size() < needed_size) { return; }

    curr_stream_->msg->append(buf.read_pos(), needed_size);
    buf.erase(needed_size);

    if (curr_stream_->msg->readable_size() == curr_stream_->msg_len) {
      if (cb) { cb(curr_stream_); }

      curr_stream_->msg->clear();
    }
    YET_LOG_ASSERT(curr_stream_->msg->readable_size() < curr_stream_->msg_len, "invalid readable size. {} {}",
                   curr_stream_->msg->readable_size(), curr_stream_->msg_len);

    header_done_ = false;

  } // parse loop
}

RtmpStreamPtr RtmpProtocol::get_or_create_stream(int csid) {
  auto &stream = csid2stream_[csid];
  if (!stream) {
    YET_LOG_DEBUG("create chunk stream. {}", csid);
    stream = std::make_shared<RtmpStream>();
    stream->msg = std::make_shared<Buffer>(BUF_INIT_LEN_RTMP_COMPLETE_MESSAGE, BUF_SHRINK_LEN_RTMP_COMPLETE_MESSAGE);
  }
  return stream;
}

}
