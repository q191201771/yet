/**
 * @file   yet.hpp
 * @author pengrl
 *
 */

#pragma once

#include <memory>
#include <cinttypes>
#include <cstddef>
#include <unordered_map>
#include <asio.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "yet_config.h"
#include "chef_base/chef_buffer.hpp"

namespace yet {

typedef asio::error_code ErrorCode;
typedef chef::buffer Buffer;
typedef std::shared_ptr<chef::buffer> BufferPtr;

extern std::shared_ptr<spdlog::logger> console;
extern Config config;

#define YET_LOG_DEBUG(...)  if (yet::console->should_log(spdlog::level::debug)) { yet::console->debug("{} - {}()#{}", fmt::format(__VA_ARGS__), __FUNCTION__, __LINE__); }
#define YET_LOG_INFO(...)  if (yet::console->should_log(spdlog::level::info)) { yet::console->info("{} - {}()#{}", fmt::format(__VA_ARGS__), __FUNCTION__, __LINE__); }
#define YET_LOG_WARN(...) if (yet::console->should_log(spdlog::level::warn)) { yet::console->warn("{} - {}()#{}", fmt::format(__VA_ARGS__), __FUNCTION__, __LINE__); }
#define YET_LOG_ERROR(...) if (yet::console->should_log(spdlog::level::err)) { yet::console->error("{} - {}()#{}", fmt::format(__VA_ARGS__), __FUNCTION__, __LINE__); }
#define YET_LOG_ASSERT(cond, ...) if (!(cond) && yet::console->should_log(spdlog::level::err)) { yet::console->error("CHEFASSERTME {} - {}()#{}", fmt::format(__VA_ARGS__), __FUNCTION__, __LINE__); }

using std::placeholders::_1;
using std::placeholders::_2;

class Server;
class RtmpServer;
class HttpFlvServer;
class Group;
class HttpFlvSub;
class HttpFlvPull;
class RtmpSession;
typedef std::shared_ptr<RtmpServer> RtmpServerPtr;
typedef std::shared_ptr<HttpFlvServer> HttpFlvServerPtr;
typedef std::shared_ptr<Group> GroupPtr;
typedef std::shared_ptr<HttpFlvSub> HttpFlvSubPtr;
typedef std::shared_ptr<HttpFlvPull> HttpFlvPullPtr;
typedef std::shared_ptr<RtmpSession> RtmpSessionPtr;

typedef std::unordered_map<std::string, GroupPtr> LiveName2Group;


}
