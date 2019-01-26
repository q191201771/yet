#include <stdio.h>
#include "yet_server.h"
#include "yet.hpp"

std::shared_ptr<yet::Server> srv;

//#if defined(__linux__) || defined(__MACH__)
//#include <signal.h>
//
//void sig_handler(int no) {
//  YET_LOG_INFO("sig {}", no);
//  srv->dispose();
//  //exit(0);
//  return;
//}
//#endif

namespace yet {
  std::shared_ptr<spdlog::logger> console;
  Config config;
}

int main(int argc, char **argv) {
  yet::console = spdlog::stdout_color_mt("yet");
  yet::console->set_level(spdlog::level::trace);

  if (argc != 4) {
    YET_LOG_ERROR("Usage: {} <rtmp port> <http flv port> <http flv pull host>", argv[0]);
    return -1;
  }
  uint16_t rtmp_port;
  uint16_t http_flv_port;
  rtmp_port = atoi(argv[1]);
  http_flv_port = atoi(argv[2]);
  yet::config.set_http_flv_pull_host(argv[3]);

  YET_LOG_DEBUG("debug log.");
  YET_LOG_INFO("info log.");
  YET_LOG_WARN("warn log.");
  YET_LOG_ERROR("error log.");
  YET_LOG_ASSERT(false, "assert log.");

//#if defined(__linux__) || defined(__MACH__)
//  signal(SIGINT, sig_handler);
//  signal(SIGUSR1, sig_handler);
//#endif

  srv = std::make_shared<yet::Server>("0.0.0.0", rtmp_port, "0.0.0.0", http_flv_port);
  std::thread thd([] {
    sleep(3);
    srv->dispose();
  });
  srv->run_loop();

  thd.join();
  YET_LOG_INFO("Bye.");
  return 0;
}
