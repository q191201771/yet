#include "http_flv_in.h"
#include "yet_inner.hpp"
#include <chef_base/chef_strings_op.hpp>

namespace yet {

HttpFlvIn::HttpFlvIn(asio::io_context &io_context, const std::string &server, const std::string &path)
  : resolver_(io_context)
  , socket_(io_context)
{
  std::ostream request_stream(&request_);
  request_stream << "GET " << path << " HTTP/1.0\r\n";
  //request_stream << "User-Agent: yet/0.0.1" << path << " HTTP/1.0\r\n";
  request_stream << "Accept: */*\r\n";
  request_stream << "Range: byte=0-\r\n";
  request_stream << "Connection: close\r\n";
  request_stream << "Host: " << server << "\r\n";
  request_stream << "Icy-MetaData: 1\r\n\r\n";

  resolver_.async_resolve(server, "http", std::bind(&HttpFlvIn::resolve_cb, this, _1, _2));
}

void HttpFlvIn::resolve_cb(const asio::error_code &ec, const asio::ip::tcp::resolver::results_type &endpoints) {
  if (ec) {
    YET_LOG_ERROR("ec:{}", ec.message());
    return;
  }

  asio::async_connect(socket_, endpoints, std::bind(&HttpFlvIn::connect_cb, shared_from_this(), _1));
}

void HttpFlvIn::connect_cb(const asio::error_code &ec) {
  if (ec) {
    YET_LOG_ERROR("ec:{}", ec.message());
    return;
  }

  asio::async_write(socket_, request_, std::bind(&HttpFlvIn::write_request_cb, shared_from_this(), _1));
}

void HttpFlvIn::write_request_cb(const asio::error_code &ec) {
  if (ec) {
    YET_LOG_ERROR("ec:{}", ec.message());
    return;
  }

  asio::async_read_until(socket_, response_, "\r\n", std::bind(&HttpFlvIn::read_status_line_cb, shared_from_this(), _1));
}

void HttpFlvIn::read_status_line_cb(const asio::error_code &ec) {
  if (ec) {
    YET_LOG_ERROR("ec:{}", ec.message());
    return;
  }

  std::istream response_stream(&response_);
  std::string http_version;
  unsigned int status_code;
  std::string status_message;
  response_stream >> http_version;
  response_stream >> status_code;
  std::getline(response_stream, status_message);
  YET_LOG_INFO("http_version:{}, status_code:{}, status_message:{}", http_version, status_code, status_message);
  if (!response_stream || chef::strings_op::has_prefix(status_message, "HTTP/") || status_code != 200) {
    YET_LOG_ERROR("Invalid status line.");
    return;
  }

  asio::async_read_until(socket_, response_, "\r\n\r\n", std::bind(&HttpFlvIn::read_headers_cb, shared_from_this(), _1));
}

void HttpFlvIn::read_headers_cb(const asio::error_code &ec) {
  if (ec) {
    YET_LOG_ERROR("ec:{}", ec.message());
    return;
  }

  std::istream response_stream(&response_);
  std::string header;
  while (std::getline(response_stream, header) && header != "\r")
    YET_LOG_INFO("{}", header);

  response_body_.reserve(4096);
  socket_.async_read_some(asio::buffer(response_body_.write_pos(), 4096), std::bind(&HttpFlvIn::read_body_cb, shared_from_this(), _1, _2));
}

void HttpFlvIn::read_body_cb(const asio::error_code &ec, std::size_t len) {
  if (ec) {
    YET_LOG_ERROR("ec:{}", ec.message());
    return;
  }
  YET_LOG_INFO("len:{} {}", len, response_body_.readable_size());
  response_body_.clear();
}

} // namespace yet

