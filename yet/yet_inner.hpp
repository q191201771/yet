/**
 * @file   yet_inner.hpp
 * @author pengrl
 *
 */

#pragma once

#include "yet_fwd.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace yet {

extern std::shared_ptr<spdlog::logger> console;

#define YET_LOG_DEBUG(...)  if (yet::console->should_log(spdlog::level::debug)) { yet::console->debug("{} - {}()#{}", fmt::format(__VA_ARGS__), __FUNCTION__, __LINE__); }
#define YET_LOG_INFO(...)  if (yet::console->should_log(spdlog::level::info)) { yet::console->info("{} - {}()#{}", fmt::format(__VA_ARGS__), __FUNCTION__, __LINE__); }
#define YET_LOG_WARN(...) if (yet::console->should_log(spdlog::level::warn)) { yet::console->warn("{} - {}()#{}", fmt::format(__VA_ARGS__), __FUNCTION__, __LINE__); }
#define YET_LOG_ERROR(...) if (yet::console->should_log(spdlog::level::err)) { yet::console->error("{} - {}()#{}", fmt::format(__VA_ARGS__), __FUNCTION__, __LINE__); }
#define YET_LOG_ASSERT(cond, ...) if (!(cond) && yet::console->should_log(spdlog::level::err)) { yet::console->error("CHEFASSERTME {} - {}()#{}", fmt::format(__VA_ARGS__), __FUNCTION__, __LINE__); }

using std::placeholders::_1;
using std::placeholders::_2;


static constexpr std::size_t EACH_READ_LEN = 4096;
static constexpr std::size_t FIXED_WRITE_BUF_RESERVE_LEN = 4096;
static constexpr std::size_t COMPLETE_MESSAGE_INIT_LEN = 4096;
static constexpr std::size_t COMPLETE_MESSAGE_SHRINK_LEN = 4194304; // 4 * 1024 * 1024
static constexpr std::size_t WINDOW_ACKNOWLEDGEMENT_SIZE = 5000000;
static constexpr std::size_t PEER_BANDWIDTH = 5000000;
static constexpr std::size_t LOCAL_CHUNK_SIZE = 4096;



static constexpr char RTMP_VERSION = '\x03';

static constexpr std::size_t fmt_2_msg_header_len[] = {11, 7, 3, 0};

static constexpr std::size_t MAX_TIMESTAMP_IN_MSG_HEADER = 0xFFFFFF;

static constexpr std::size_t RTMP_CSID_PROTOCOL_CONTROL = 2;
static constexpr std::size_t RTMP_CSID_OVER_CONNECTION  = 3;
static constexpr std::size_t RTMP_CSID_OVER_STREAM      = 5;

static constexpr std::size_t RTMP_MSID_WHILE_PROTOCOL_CONTROL_MESSAGE = 0;

// 1,2,3,5,6 for protocol control message
static constexpr std::size_t MSG_TYPE_ID_SET_CHUNK_SIZE       = 0x01;
static constexpr std::size_t MSG_TYPE_ID_ABORT                = 0x02;
static constexpr std::size_t MSG_TYPE_ID_ACK                  = 0x03;
static constexpr std::size_t MSG_TYPE_ID_WIN_ACK_SIZE         = 0x05;
static constexpr std::size_t MSG_TYPE_ID_BANDWIDTH            = 0x06;

static constexpr std::size_t MSG_TYPE_ID_USER_CONTROL         = 0x04;
static constexpr std::size_t MSG_TYPE_ID_AUDIO                = 0x08;
static constexpr std::size_t MSG_TYPE_ID_VIDEO                = 0x09;
static constexpr std::size_t MSG_TYPE_ID_DATA_MESSAGE_AMF0    = 0x12;
static constexpr std::size_t MSG_TYPE_ID_COMMAND_MESSAGE_AMF0 = 0x14;

static constexpr std::size_t TRANSACTION_ID_CONNECT        = 1;
static constexpr std::size_t TRANSACTION_ID_RELEASE_STREAM = 2;
static constexpr std::size_t TRANSACTION_ID_FC_PUBLISH     = 3;
static constexpr std::size_t TRANSACTION_ID_CREATE_STREAM  = 4; // send not recv
static constexpr std::size_t TRANSACTION_ID_PUBLISH        = 5;

static constexpr std::size_t PEER_BANDWITH_LIMIT_TYPE_HARD    = 0;
static constexpr std::size_t PEER_BANDWITH_LIMIT_TYPE_SOFT    = 1;
static constexpr std::size_t PEER_BANDWITH_LIMIT_TYPE_DYNAMIC = 2;

} // namespace yet

