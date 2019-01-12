/**
 * @file   http_flv_server.h
 * @author pengrl
 *
 */

#pragma once

#include <unordered_map>
#include <string>
#include "yet_fwd.hpp"
#include "http_flv_session.h"

namespace yet {

class HttpFlvServer : public std::enable_shared_from_this<HttpFlvServer>
                    , public HttpFlvSessionObserver
{
  public:
    HttpFlvServer(const std::string &listen_ip, uint16_t listen_port);
    virtual ~HttpFlvServer() {}

    void run_loop();

  private:
    void do_accept();
    void accept_cb(ErrorCode ec, asio::ip::tcp::socket socket);

  private:
    virtual void on_request(HttpFlvSessionPtr hfs, const std::string &uri, const std::string &app_name,
                            const std::string &live_name, const std::string &host);

  private:
    HttpFlvServer(const HttpFlvServer &);
    HttpFlvServer &operator=(const HttpFlvServer &);

  private:
    std::string             listen_ip_;
    uint16_t                listen_port_;
    asio::io_context        io_ctx_;
    asio::ip::tcp::acceptor acceptor_;
    std::unordered_map<std::string, GroupPtr> live_name_2_group_;
};

}
