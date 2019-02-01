/**
 * @file   yet_log.h
 * @author pengrl
 *
 */

#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#define YET_LOG_DEBUG(...)        if (yet::Log::instance()->should_log(spdlog::level::debug)) { yet::Log::instance()->debug("{} - {}()#{}", fmt::format(__VA_ARGS__), __FUNCTION__, __LINE__); }
#define YET_LOG_INFO(...)         if (yet::Log::instance()->should_log(spdlog::level::info)) { yet::Log::instance()->info("{} - {}()#{}", fmt::format(__VA_ARGS__), __FUNCTION__, __LINE__); }
#define YET_LOG_WARN(...)         if (yet::Log::instance()->should_log(spdlog::level::warn)) { yet::Log::instance()->warn("{} - {}()#{}", fmt::format(__VA_ARGS__), __FUNCTION__, __LINE__); }
#define YET_LOG_ERROR(...)        if (yet::Log::instance()->should_log(spdlog::level::err)) { yet::Log::instance()->error("{} - {}()#{}", fmt::format(__VA_ARGS__), __FUNCTION__, __LINE__); }
#define YET_LOG_ASSERT(cond, ...) if (!(cond) && yet::Log::instance()->should_log(spdlog::level::err)) { yet::Log::instance()->error("CHEFASSERTME {} - {}()#{}", fmt::format(__VA_ARGS__), __FUNCTION__, __LINE__); }

namespace yet {

class Log {
  public:
    static std::shared_ptr<spdlog::logger> instance();

    // just for mem check
    static void dispose();

  private:
    Log() {}
    Log(const Log &) = delete;
    Log &operator=(const Log &) = delete;

  private:
    static std::shared_ptr<spdlog::logger> core_;
};

}
