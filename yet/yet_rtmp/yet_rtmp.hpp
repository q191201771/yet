/**
 * @file   yet_rtmp.hpp
 * @author pengrl
 *
 */

#pragma once

#include "yet_common/yet_common.hpp"

// config
namespace yet {

static constexpr std::size_t RTMP_LOCAL_CHUNK_SIZE = 4096;
static constexpr std::size_t RTMP_PEER_BANDWIDTH = 5000000;
static constexpr std::size_t RTMP_WINDOW_ACKNOWLEDGEMENT_SIZE = 5000000;

}

// const
namespace yet {

// basic header   3
// message header 11
// extended ts    4
static constexpr std::size_t RTMP_MAX_HEADER_LEN = 18;

static constexpr std::size_t RTMP_MAX_TIMESTAMP_IN_MSG_HEADER = 0xFFFFFF;

static constexpr std::size_t RTMP_C0C1_LEN = 1537;
static constexpr std::size_t RTMP_S0S1_LEN = 1537;
static constexpr std::size_t RTMP_C2_LEN   = 1536;
static constexpr std::size_t RTMP_S2_LEN   = 1536;

static constexpr char RTMP_VERSION = '\x03';

static constexpr std::size_t RTMP_FMT_2_MSG_HEADER_LEN[] = {11, 7, 3, 0};

static constexpr int CHUNK_HEADER_SIZE_TYPE0 = 12;

static constexpr std::size_t RTMP_MAX_CSID = 65599;

static constexpr std::size_t RTMP_CSID_PROTOCOL_CONTROL = 2;
static constexpr std::size_t RTMP_CSID_OVER_CONNECTION  = 3;
static constexpr std::size_t RTMP_CSID_OVER_STREAM      = 5;
static constexpr std::size_t RTMP_CSID_AMF              = 5; // send meta
static constexpr std::size_t RTMP_CSID_AUDIO            = 6;
static constexpr std::size_t RTMP_CSID_VIDEO            = 7;

// 1,2,3,5,6 for protocol control message
static constexpr std::size_t RTMP_MSG_TYPE_ID_SET_CHUNK_SIZE       = 0x01;
static constexpr std::size_t RTMP_MSG_TYPE_ID_ABORT                = 0x02;
static constexpr std::size_t RTMP_MSG_TYPE_ID_ACK                  = 0x03;
static constexpr std::size_t RTMP_MSG_TYPE_ID_WIN_ACK_SIZE         = 0x05;
static constexpr std::size_t RTMP_MSG_TYPE_ID_BANDWIDTH            = 0x06;

static constexpr std::size_t RTMP_MSG_TYPE_ID_USER_CONTROL         = 0x04;
static constexpr std::size_t RTMP_MSG_TYPE_ID_AUDIO                = 0x08;
static constexpr std::size_t RTMP_MSG_TYPE_ID_VIDEO                = 0x09;
static constexpr std::size_t RTMP_MSG_TYPE_ID_DATA_MESSAGE_AMF0    = 0x12; // meta
static constexpr std::size_t RTMP_MSG_TYPE_ID_COMMAND_MESSAGE_AMF0 = 0x14;

static constexpr std::size_t RTMP_MSID_WHILE_PROTOCOL_CONTROL_MESSAGE = 0;
static constexpr std::size_t RTMP_MSID                                = 1;

static constexpr int USER_CONTROL_EVENT_TYPE_STREAM_BEGIN  = 0;
static constexpr int USER_CONTROL_EVENT_TYPE_STREAM_EOF    = 1;
static constexpr int USER_CONTROL_EVENT_TYPE_PING_REQUEST  = 6;
static constexpr int USER_CONTROL_EVENT_TYPE_PING_RESPONSE = 7;

static constexpr std::size_t RTMP_TRANSACTION_ID_CONNECT        = 1;
static constexpr std::size_t RTMP_TRANSACTION_ID_RELEASE_STREAM = 2;
static constexpr std::size_t RTMP_TRANSACTION_ID_FC_PUBLISH     = 3;
static constexpr std::size_t RTMP_TRANSACTION_ID_CREATE_STREAM  = 4; // send not recv
static constexpr std::size_t RTMP_TRANSACTION_ID_PLAY           = 4; // play handler
static constexpr std::size_t RTMP_TRANSACTION_ID_PUBLISH        = 5;

static constexpr std::size_t RTMP_PEER_BANDWITH_LIMIT_TYPE_HARD    = 0;
static constexpr std::size_t RTMP_PEER_BANDWITH_LIMIT_TYPE_SOFT    = 1;
static constexpr std::size_t RTMP_PEER_BANDWITH_LIMIT_TYPE_DYNAMIC = 2;

static constexpr std::size_t RTMP_DEFAULT_CHUNK_SIZE = 128;

}

namespace yet {

struct RtmpHeader {
  uint32_t csid;
  uint32_t timestamp;
  uint32_t msg_len;
  uint32_t msg_type_id;
  uint32_t msg_stream_id;

  bool is_audio() { return msg_type_id == RTMP_MSG_TYPE_ID_AUDIO; }
};

struct RtmpStream {
  RtmpHeader  header;
  std::size_t msg_len;
  BufferPtr   msg;
  uint32_t    timestamp_abs;
};

}

