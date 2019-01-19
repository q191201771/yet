#include <stdio.h>
#include "rtmp_server.h"
#include "http_flv_server.h"
#include "yet_inner.hpp"

namespace yet {
  std::shared_ptr<spdlog::logger> console;
  Config config;
}

int main(int argc, char **argv) {
  if (argc != 4) {
    YET_LOG_ERROR("Usage: {} <rtmp port> <http flv port> <http flv pull host>", argv[0]);
    return -1;
  }
  uint16_t rtmp_port;
  uint16_t http_flv_port;
  rtmp_port = atoi(argv[1]);
  http_flv_port = atoi(argv[2]);
  yet::config.set_http_flv_pull_host(argv[3]);

  yet::console = spdlog::stdout_color_mt("yet");
  yet::console->set_level(spdlog::level::trace);
  //YET_LOG_INFO("> main.");
  YET_LOG_DEBUG("debug log.");
  YET_LOG_INFO("info log.");
  YET_LOG_WARN("warn log.");
  YET_LOG_ERROR("error log.");
  YET_LOG_ASSERT(false, "assert log.");

  auto http_flv_srv = std::make_shared<yet::HttpFlvServer>("0.0.0.0", http_flv_port);
  http_flv_srv->run_loop();

  //yet::RTMPServer rtmp_srv("0.0.0.0", rtmp_port);
  //rtmp_srv.run_loop();

  return 0;
}
