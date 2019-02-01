/**
 * @file   yet_common.hpp
 * @author pengrl
 *
 */

#pragma once

#include <functional>
#include "chef_base/chef_buffer.hpp"
#include "yet_common/yet_log.h"

namespace yet {

typedef chef::buffer Buffer;
typedef std::shared_ptr<Buffer> BufferPtr;

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

}
