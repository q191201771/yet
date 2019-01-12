/**
 * @file   http_flv_session.h
 * @author pengrl
 *
 */

#pragma once

#include "yet_fwd.hpp"

namespace yet {

class HttpFlvSessionObserver {
  public:
    virtual ~HttpFlvSessionObserver() {}

    virtual void on_request(HttpFlvSessionPtr hfs, const std::string &uri, const std::string &app_name,
                            const std::string &live_name, const std::string &host) = 0;
};

class HttpFlvSession : public std::enable_shared_from_this<HttpFlvSession> {
  public:
    explicit HttpFlvSession(asio::ip::tcp::socket socket, std::weak_ptr<HttpFlvSessionObserver> obs);
    ~HttpFlvSession();

    void start();

    void request_handler(ErrorCode ec, std::size_t len);

  private:
    HttpFlvSession(const HttpFlvSession &) = delete;
    HttpFlvSession &operator=(const HttpFlvSession &) = delete;

  private:
    asio::ip::tcp::socket socket_;
    std::weak_ptr<HttpFlvSessionObserver> obs_;
    asio::streambuf request_buf_;
};

}
