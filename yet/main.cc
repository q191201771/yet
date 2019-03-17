#include <stdio.h>
#include "yet.hpp"
#include "yet_server.h"
#include "yet_config.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <conf file>\n", argv[0]);
    return -1;
  }

  auto res = yet::Config::instance()->load_conf_file(argv[1]);
  if (!res) { return -1; }

  yet::Log::instance();

  auto srv = std::make_shared<yet::Server>(yet::Config::instance()->rtmp_server_ip(),
                                           yet::Config::instance()->rtmp_server_port(),
                                           yet::Config::instance()->http_flv_server_ip(),
                                           yet::Config::instance()->http_flv_server_port());
  int run_duration_sec = 0;
  std::thread thd([&] {
    if (run_duration_sec != 0) {
      sleep(run_duration_sec);
      srv->dispose();
    }
  });
  srv->run_loop();

  thd.join();

  srv.reset();

  YET_LOG_INFO("Bye.");
  yet::Config::dispose();
  yet::Log::dispose();
  return 0;
}
