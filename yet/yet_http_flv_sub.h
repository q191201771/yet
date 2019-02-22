/**
 * @file   yet_http_flv_sub.h
 * @author pengrl
 *
 */

#pragma once

#include <queue>
#include <asio.hpp>
#include "yet.hpp"
#include "yet_http_flv/yet_http_flv_buffer_t.hpp"

namespace yet {

//class HttpFlvSubObserver {
//  public:
//    virtual ~HttpFlvSubObserver() {}
//
//    virtual void on_http_flv_request(HttpFlvSubPtr sub, const std::string &uri, const std::string &app_name,
//                                     const std::string &live_name, const std::string &host) = 0;
//};

class HttpFlvSub : public std::enable_shared_from_this<HttpFlvSub> {
  public:
    typedef std::function<void(HttpFlvSubPtr http_flv_sub, const std::string &uri, const std::string &app_name,
                               const std::string &live_name, const std::string &host)> HttpFlvSubCb;

    typedef std::function<void(HttpFlvSubPtr http_flv_sub)> HttpFlvSubEventCb;

  public:
    explicit HttpFlvSub(asio::ip::tcp::socket socket);
    ~HttpFlvSub();

  public:
    void set_sub_cb(HttpFlvSubCb cb);
    // TODO choice cb or group cb
    void set_close_cb(HttpFlvSubEventCb cb);

    void start();

    void async_send(BufferPtr buf, const std::vector<FlvTagInfo> &tis);

    void set_group(std::weak_ptr<Group> group);

    void dispose() {}

  private:
    void close();

  private:
    void request_handler(const ErrorCode &ec, std::size_t len);
    void do_send_http_headers();
    void send_http_headers_cb(const ErrorCode &ec);
    void do_send_flv_header();
    void send_flv_header_cb(const ErrorCode &ec);
    void send_metadata_cb(const ErrorCode &ec);
    void send_avc_header_cb(const ErrorCode &ec);
    void send_aac_header_cb(const ErrorCode &ec);
    void do_send();
    void send_cb(const ErrorCode &ec, std::size_t len);

  private:
    HttpFlvSub(const HttpFlvSub &) = delete;
    HttpFlvSub &operator=(const HttpFlvSub &) = delete;

  private:
    asio::ip::tcp::socket             socket_;
    HttpFlvSubCb                      sub_cb_;
    HttpFlvSubEventCb                 close_cb_;
    std::weak_ptr<Group>              group_;
    asio::streambuf                   request_buf_;
    std::queue<BufferPtr>             send_buffers_;
    bool                              is_bc_ready_ = false;
    bool                              sent_first_key_frame_ = false;
};

}
