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

class Group;
class HttpFlvSession;
class HttpFlvIn;
typedef std::shared_ptr<Group> GroupPtr;
typedef std::shared_ptr<HttpFlvSession> HttpFlvSessionPtr;
typedef std::shared_ptr<HttpFlvIn> HttpFlvInPtr;

static constexpr std::size_t C0C1_LEN = 1537;
static constexpr std::size_t S0S1_LEN = 1537;
static constexpr std::size_t C2_LEN   = 1536;
static constexpr std::size_t S2_LEN   = 1536;

static constexpr std::size_t DEFAULT_CHUNK_SIZE = 128;
}
