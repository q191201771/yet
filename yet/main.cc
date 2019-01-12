#include <stdio.h>
#include "rtmp_server.h"
#include "http_flv_server.h"
#include "yet_inner.hpp"

namespace yet {
  std::shared_ptr<spdlog::logger> console;
}

int main(int argc, char **argv) {
  uint16_t port=1935;
  if (argc == 2) {
    port = atoi(argv[1]);
  }

  yet::console = spdlog::stdout_color_mt("yet");
  yet::console->set_level(spdlog::level::trace);
  //YET_LOG_INFO("> main.");
  YET_LOG_DEBUG("debug log.");
  YET_LOG_INFO("info log.");
  YET_LOG_WARN("warn log.");
  YET_LOG_ERROR("error log.");
  YET_LOG_ASSERT(false, "assert log.");

  auto http_flv_srv = std::make_shared<yet::HttpFlvServer>("0.0.0.0", 8080);
  http_flv_srv->run_loop();

  //yet::RTMPServer rtmp_srv("0.0.0.0", port);
  //rtmp_srv.run_loop();

  return 0;
}
