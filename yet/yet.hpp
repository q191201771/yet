/**
 * @file   yet.hpp
 * @author pengrl
 *
 */

#pragma once

#include <cinttypes>
#include <cstddef>
#include <system_error>
#include "yet_common/yet_common.hpp"
#include "yet_common/yet_log.h"

// config
namespace yet {

static constexpr size_t BUF_INIT_LEN_HTTP_FLV_PULL_EACH_READ   = 16384;
static constexpr size_t BUF_SHRINK_LEN_HTTP_FLV_PULL_EACH_READ = 2147483647;
static constexpr size_t BUF_INIT_LEN_RTMP_EACH_READ            = 16384;
static constexpr size_t BUF_SHRINK_LEN_RTMP_EACH_READ          = 2147483647;
// @NOTICE len of rtmp-write only for non-av data
static constexpr size_t BUF_INIT_LEN_RTMP_WRITE                = 4096;
static constexpr size_t BUF_SHRINK_LEN_RTMP_WRITE              = 2147483647;
static constexpr size_t CLEAN_EMPTY_GROUP_INTERVAL_MSEC        = 30000;

}

// const
namespace yet {

using ErrorCode = std::error_code;

class Server;
class RtmpServer;
class HttpFlvServer;
class Group;
class HttpFlvSub;
class HttpFlvPull;
class RtmpSessionBase;
class RtmpSessionServer;
class RtmpSessionClient;
using ServerPtr            = std::shared_ptr<Server>;
using RtmpServerPtr        = std::shared_ptr<RtmpServer> ;
using HttpFlvServerPtr     = std::shared_ptr<HttpFlvServer>;
using GroupPtr             = std::shared_ptr<Group>;
using HttpFlvSubPtr        = std::shared_ptr<HttpFlvSub>;
using HttpFlvPullPtr       = std::shared_ptr<HttpFlvPull>;
using RtmpSessionBasePtr   = std::shared_ptr<RtmpSessionBase>;
using RtmpSessionServerPtr = std::shared_ptr<RtmpSessionServer>;
using RtmpSessionClientPtr = std::shared_ptr<RtmpSessionClient>;

class AmfObjectItemMap;
using AmfObjectItemMapPtr = std::shared_ptr<AmfObjectItemMap>;

}
