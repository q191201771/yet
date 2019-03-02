#include <stdio.h>
#include "yet.hpp"
#include "yet_server.h"
#include "yet_config.h"

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

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <conf file>\n", argv[0]);
    return -1;
  }

  auto res = yet::Config::instance()->load_conf_file(argv[1]);
  if (!res) { return -1; }

  yet::Log::instance();

//#if defined(__linux__) || defined(__MACH__)
//  signal(SIGINT, sig_handler);
//  signal(SIGUSR1, sig_handler);
//#endif

  srv = std::make_shared<yet::Server>(yet::Config::instance()->rtmp_server_ip(),
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
