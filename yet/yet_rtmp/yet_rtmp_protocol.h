/**
 * @file   yet_rtmp_protocol.h
 * @author pengrl
 *
 */

#pragma once

#include <functional>
#include "yet_rtmp.hpp"

namespace yet {

class RtmpProtocol {
  public:
    using CompleteMessageCb = std::function<void(RtmpStreamPtr stream)>;

  public:
    //void set_complete_message_cb(CompleteMessageCb cb);
    void update_peer_chunk_size(size_t val);

    /// <try_compose> is rtmp protocol state machine.
    /// you can append raw data to <buf>(let's say read from network), call <try_compose> to parse it.
    /// and if <buf> contains complete messages, this func will move those message from <buf> to RtmpStreamPtr and <cb>
    /// will be call back.
    void try_compose(Buffer &buf, CompleteMessageCb cb);

  private:
    RtmpStreamPtr get_or_create_stream(int csid);

  private:
    using Csid2Stream = std::unordered_map<int, RtmpStreamPtr>;

  private:
    size_t       peer_chunk_size_ = RTMP_DEFAULT_CHUNK_SIZE;
    //CompleteMessageCb complete_message_cb_;
    bool              header_done_=false; // CHEFTODO rename
    int               curr_csid_;
    RtmpStreamPtr     curr_stream_;
    Csid2Stream       csid2stream_;
};

}
