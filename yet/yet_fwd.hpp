/**
 * @file   yet_fwd.hpp
 * @author pengrl
 *
 */

#pragma once

#include <cinttypes>
#include <cstddef>
#include <chef_base/chef_buffer.hpp>
#include <asio.hpp>

namespace yet {

typedef asio::error_code ErrorCode;
typedef chef::buffer Buffer;
typedef std::shared_ptr<chef::buffer> BufferPtr;

class Group;
class HttpFlvSub;
class HttpFlvPull;
typedef std::shared_ptr<Group> GroupPtr;
typedef std::shared_ptr<HttpFlvSub> HttpFlvSubPtr;
typedef std::shared_ptr<HttpFlvPull> HttpFlvPullPtr;



static constexpr std::size_t FLV_TAG_HEADER_LEN = 11;

static constexpr std::size_t FLV_TAG_HEADER_TYPE_AUDIO       = 8;
static constexpr std::size_t FLV_TAG_HEADER_TYPE_VIDEO       = 9;
static constexpr std::size_t FLV_TAG_HEADER_TYPE_SCRIPT_DATA = 18;



static constexpr std::size_t C0C1_LEN = 1537;
static constexpr std::size_t S0S1_LEN = 1537;
static constexpr std::size_t C2_LEN   = 1536;
static constexpr std::size_t S2_LEN   = 1536;

static constexpr std::size_t DEFAULT_CHUNK_SIZE = 128;
}
