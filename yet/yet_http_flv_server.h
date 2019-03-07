/**
 * @file   yet_http_flv_server.h
 * @author pengrl
 *
 */

#pragma once

#include <unordered_map>
#include <string>
#include <asio.hpp>
#include "yet.hpp"
#include "yet_http_flv_session_sub.h"

namespace yet {

class HttpFlvServer : public std::enable_shared_from_this<HttpFlvServer>
{
  public:
    HttpFlvServer(asio::io_context &io_ctx, const std::string &listen_ip, uint16_t listen_port, Server *server);
    virtual ~HttpFlvServer();

    void start();

    void dispose();

  private:
    void do_accept();
    void accept_cb(const ErrorCode &ec, asio::ip::tcp::socket socket);

  private:
    void on_http_flv_request(HttpFlvSubPtr sub, const std::string &uri, const std::string &app_name,
                             const std::string &stream_name, const std::string &host);

  private:
    HttpFlvServer(const HttpFlvServer &) = delete;
    HttpFlvServer &operator=(const HttpFlvServer &) = delete;

  private:
    asio::io_context        &io_ctx_;
    const std::string       listen_ip_;
    const uint16_t          listen_port_;
    Server                  *server_;
    asio::ip::tcp::acceptor acceptor_;
};

}
