/**
 * @file   http_flv_in.hpp
 * @author pengrl
 *
 */

#pragma once

#include "yet_fwd.hpp"

namespace yet {

class HttpFlvIn : public std::enable_shared_from_this<HttpFlvIn> {
  public:
    HttpFlvIn(asio::io_context& io_context, const std::string& server, const std::string& path);

  private:
    void resolve_cb(const asio::error_code &ec, const asio::ip::tcp::resolver::results_type &endpoints);
    void connect_cb(const asio::error_code &ec);
    void write_request_cb(const asio::error_code &ec);
    void read_status_line_cb(const asio::error_code &ec);
    void read_headers_cb(const asio::error_code &ec);
    void read_body_cb(const asio::error_code &ec, std::size_t len);

  private:
    asio::ip::tcp::resolver resolver_;
    asio::ip::tcp::socket socket_;
    asio::streambuf request_;
    asio::streambuf response_;
    chef::buffer response_body_;
};

}
