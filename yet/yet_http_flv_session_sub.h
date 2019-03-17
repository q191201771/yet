/**
 * @file   yet_http_flv_session_sub.h
 * @author pengrl
 *
 */

#pragma once

#include <queue>
#include <asio.hpp>
#include "chef_base/chef_snippet.hpp"
#include "yet.hpp"

namespace yet {

class HttpFlvSub : public std::enable_shared_from_this<HttpFlvSub> {
  public:
    using HttpFlvSubCb = std::function<void(HttpFlvSubPtr http_flv_sub, const std::string &uri, const std::string &app_name,
                                            const std::string &stream_name, const std::string &host)>;

    using HttpFlvSubEventCb = std::function<void(HttpFlvSubPtr http_flv_sub)>;

  public:
    explicit HttpFlvSub(asio::ip::tcp::socket socket);
    ~HttpFlvSub();

  public:
    void set_sub_cb(HttpFlvSubCb cb);
    void set_close_cb(HttpFlvSubEventCb cb);

    void start();

    void async_send(BufferPtr buf);

    void dispose() {}

  private:
    void close();

  private:
    void request_handler(const ErrorCode &ec, size_t len);
    void do_send_http_headers();
    void do_send_flv_header();
    void do_send();

  private:
    HttpFlvSub(const HttpFlvSub &) = delete;
    HttpFlvSub &operator=(const HttpFlvSub &) = delete;

  public:
    CHEF_PROPERTY_WITH_INIT_VALUE(bool, has_sent_metadata, false);
    CHEF_PROPERTY_WITH_INIT_VALUE(bool, has_sent_audio, false);
    CHEF_PROPERTY_WITH_INIT_VALUE(bool, has_sent_video, false);
    CHEF_PROPERTY_WITH_INIT_VALUE(bool, has_sent_key_frame, false);

  private:
    asio::ip::tcp::socket             socket_;
    HttpFlvSubCb                      sub_cb_;
    HttpFlvSubEventCb                 close_cb_;
    asio::streambuf                   request_buf_;
    std::queue<BufferPtr>             send_buffers_;
};

}
