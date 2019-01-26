/**
 * @file   yet.hpp
 * @author pengrl
 * @date   20190127
 *
 */

#pragma once

#include <memory>
#include <cinttypes>
#include <cstddef>
#include <unordered_map>
#include "chef_base/chef_buffer.hpp"
#include "yet_log.h"

namespace yet {

typedef std::error_code ErrorCode;

typedef chef::buffer Buffer;
typedef std::shared_ptr<Buffer> BufferPtr;

using std::placeholders::_1;
using std::placeholders::_2;

class Server;
class RtmpServer;
class HttpFlvServer;
class Group;
class HttpFlvSub;
class HttpFlvPull;
class RtmpSession;
typedef std::shared_ptr<Server> ServerPtr;
typedef std::shared_ptr<RtmpServer> RtmpServerPtr;
typedef std::shared_ptr<HttpFlvServer> HttpFlvServerPtr;
typedef std::shared_ptr<Group> GroupPtr;
typedef std::shared_ptr<HttpFlvSub> HttpFlvSubPtr;
typedef std::shared_ptr<HttpFlvPull> HttpFlvPullPtr;
typedef std::shared_ptr<RtmpSession> RtmpSessionPtr;

typedef std::unordered_map<std::string, GroupPtr> LiveName2Group;

}
