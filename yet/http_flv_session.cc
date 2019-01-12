#include "http_flv_session.h"
#include "http_flv_in.h"
#include "yet_inner.hpp"
#include <chef_base/chef_strings_op.hpp>
#include <string>

namespace yet {

HttpFlvSession::HttpFlvSession(asio::ip::tcp::socket socket, std::weak_ptr<HttpFlvSessionObserver> obs)
  : socket_(std::move(socket))
  , obs_(obs)
  , request_buf_(1024)
{
  YET_LOG_INFO("HttpFlvSession() {}.", static_cast<void *>(this));
}

HttpFlvSession::~HttpFlvSession() {
  YET_LOG_INFO("~HttpFlvSession() {}.", static_cast<void *>(this));
}


void HttpFlvSession::start() {
  asio::async_read_until(socket_, request_buf_, "\r\n\r\n", std::bind(&HttpFlvSession::request_handler, shared_from_this(), _1, _2));
}

void HttpFlvSession::request_handler(ErrorCode ec, std::size_t len) {
  YET_LOG_DEBUG("ec:{} len:{}", ec.message(), len);

  using namespace chef;

  std::string status_line;
  std::string uri;
  std::string app_name;
  std::string live_name;
  std::string host;

  std::istream request_stream(&request_buf_);
  if (!std::getline(request_stream, status_line) || status_line.empty() || status_line.back() != '\r') {
    YET_LOG_ERROR("request status line failed. {}", status_line);
    return;
  }
  YET_LOG_DEBUG("status line:{}", status_line);
  auto sls = strings_op::split(status_line, ' ');
  if (sls.size() != 3 || strings_op::to_upper(sls[0]) != "GET") {
    YET_LOG_ERROR("request status line failed. {}", status_line);
    return;
  }
  uri = sls[1];
  auto ukv = strings_op::split(uri, '/', false);
  if (ukv.empty() || ukv.size() < 2 || !chef::strings_op::has_suffix(ukv[ukv.size()-1], ".flv")) {
    YET_LOG_ERROR("request status line failed. {}", status_line);
    return;
  }
  app_name = ukv[ukv.size()-2];
  live_name = chef::strings_op::trim_suffix(ukv[ukv.size()-1], ".flv");

  std::string header;
  while (std::getline(request_stream, header) && !header.empty() && header != "\r") {
    header.erase(header.length()-1); // erase '\r'
    YET_LOG_DEBUG("{}", header);

    auto hkv = strings_op::split(header, ':', false, true);
    if (hkv.size() != 2) { continue; }

    auto k = strings_op::to_lower(strings_op::trim_right(strings_op::trim_left(hkv[0])));
    auto v = strings_op::to_lower(strings_op::trim_right(strings_op::trim_left(hkv[1])));
    if (k == "host") {
      host = v;
      break;
    }
  }
  YET_LOG_INFO("uri:{}, app:{}, live:{}, host:{}", uri, app_name, live_name, host);

  if (auto obs = obs_.lock()) {
    obs->on_request(shared_from_this(), uri, app_name, live_name, host);
  }
}

}
