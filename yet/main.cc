#include <stdio.h>
#include "rtmp_server.h"
#include "yet_inner.hpp"

namespace yet {
  std::shared_ptr<spdlog::logger> console;
}

int main() {
  yet::console = spdlog::stdout_color_mt("yet");
  YET_LOG_INFO("> main.");

  yet::RTMPServer srv("127.0.0.1", 1935);
  srv.run_loop();

  return 0;
}
