/**
 * @file   yet_server.h
 * @author pengrl
 * @date   20190127
 *
 */

#pragma once

#include <asio.hpp>
#include "yet.hpp"

namespace yet {

class Server {
  public:
    Server(const std::string &rtmp_listen_ip, uint16_t rtmp_listen_port,
           const std::string &http_flv_listen_ip, uint16_t http_flv_listen_port);

    ~Server();

    void run_loop();
    void dispose();

    GroupPtr get_or_create_group(const std::string &live_name);

  private:
    Server(const Server &);
    Server &operator=(const Server &);

  private:
    asio::io_context io_ctx_;
    RtmpServerPtr    rtmp_server_;
    HttpFlvServerPtr http_flv_server_;
    LiveName2Group   live_name_2_group_;
};

}
