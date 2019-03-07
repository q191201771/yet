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

using Buffer = chef::buffer;
using BufferPtr = std::shared_ptr<Buffer>;

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_4;
using std::placeholders::_5;

}
