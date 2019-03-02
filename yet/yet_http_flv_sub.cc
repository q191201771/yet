#include "yet_http_flv_sub.h"
#include <string>
#include <asio.hpp>
#include "yet_http_flv_pull.h"
#include "yet.hpp"
#include "chef_base/chef_strings_op.hpp"
#include "chef_base/chef_stuff_op.hpp"

// CHEFTODO dup code with yet_rtmp_session.cc
#define SNIPPET_ENTER_CB \
  do { \
    if (!ec) { \
    } else { \
      YET_LOG_ERROR("[{}] {} ec:{}, len:{}", (void *)this, __func__, ec.message(), len); \
      if (ec == asio::error::eof) { \
        YET_LOG_INFO("[{}] close by peer.", (void *)this); \
        close(); \
      } else if (ec == asio::error::broken_pipe) { \
        YET_LOG_ERROR("[{}] broken pipe.", (void *)this); \
      } \
      return; \
    } \
  } while(0);

namespace yet {

HttpFlvSub::HttpFlvSub(asio::ip::tcp::socket socket)
  : socket_(std::move(socket))
  , request_buf_(1024)
{
  YET_LOG_DEBUG("[{}] new HttpFlvSub.", (void *)this);
}

HttpFlvSub::~HttpFlvSub() {
  YET_LOG_DEBUG("[{}] delete HttpFlvSub.", (void *)this);
}

void HttpFlvSub::set_sub_cb(HttpFlvSubCb cb) { sub_cb_ = cb; }

void HttpFlvSub::set_close_cb(HttpFlvSubEventCb cb) { close_cb_ = cb; }

void HttpFlvSub::start() {
  asio::async_read_until(socket_, request_buf_, "\r\n\r\n", std::bind(&HttpFlvSub::request_handler, shared_from_this(), _1, _2));
}

void HttpFlvSub::request_handler(const ErrorCode &ec, std::size_t len) {
  SNIPPET_ENTER_CB;

  using namespace chef;

  std::string status_line;
  std::string uri;
  std::string app_name;
  std::string live_name;
  std::string host;

  std::istream request_stream(&request_buf_);
  if (!std::getline(request_stream, status_line) || status_line.empty() || status_line.back() != '\r') {
    YET_LOG_ERROR("request status line failed. <{}> <{}>", status_line, chef::stuff_op::bytes_to_hex((const uint8_t *)status_line.c_str(), status_line.length()));
    return;
  }
  YET_LOG_DEBUG("[{}] status line:{}", (void *)this, status_line.erase(status_line.length()-1));
  auto sls = strings_op::split(status_line, ' ');
  if (sls.size() != 3 || strings_op::to_upper(sls[0]) != "GET") {
    YET_LOG_ERROR("request status line failed. <{}> <{}>", status_line, chef::stuff_op::bytes_to_hex((const uint8_t *)status_line.c_str(), status_line.length()));
    return;
  }
  uri = sls[1];
  auto ukv = strings_op::split(uri, '/', false);
  if (ukv.empty() || ukv.size() < 2 || !chef::strings_op::has_suffix(ukv[ukv.size()-1], ".flv")) {
    YET_LOG_ERROR("request status line failed. <{}> <{}>", status_line, chef::stuff_op::bytes_to_hex((const uint8_t *)status_line.c_str(), status_line.length()));
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
  YET_LOG_DEBUG("[{}] uri:{}, app:{}, live:{}, host:{}", (void *)this, uri, app_name, live_name, host);

  if (sub_cb_) { sub_cb_(shared_from_this(), uri, app_name, live_name, host); }

  do_send_http_headers();
}

void HttpFlvSub::do_send_http_headers() {
  auto self(shared_from_this());
  asio::async_write(socket_,
                    asio::buffer(FLV_HTTP_HEADERS, FLV_HTTP_HEADERS_LEN),
                    [this, self](const ErrorCode &ec, std::size_t len) {
                      SNIPPET_ENTER_CB;
                      do_send_flv_header();
                    });
}

void HttpFlvSub::do_send_flv_header() {
  auto self(shared_from_this());
  asio::async_write(socket_,
                    asio::buffer(FLV_HEADER_BUF_13, 13),
                    [this, self](const ErrorCode &ec, std::size_t len) {
                      SNIPPET_ENTER_CB;
                    });
}

void HttpFlvSub::async_send(BufferPtr buf) {
  auto is_empty = send_buffers_.empty();
  send_buffers_.push(buf);
  if (is_empty) { do_send(); }
}

void HttpFlvSub::async_send(BufferPtr buf, const std::vector<FlvTagInfo> &tis) {
  if (!sent_first_key_frame_) {
    if (tis.empty()) { return; }

    for (auto &ti : tis) {
      if (ti.tag_type == FLVTAGTYPE_VIDEO) {
        if (*(ti.tag_pos + 11) == 0x17) {
          send_buffers_.push(std::make_shared<Buffer>(ti.tag_pos, buf->write_pos()-ti.tag_pos));
          do_send();
          sent_first_key_frame_ = true;
          YET_LOG_DEBUG("key");
          break;
        }
      }
    }
    return;
  }

  async_send(buf);
}

void HttpFlvSub::do_send() {
  auto buf = send_buffers_.front();
  auto self(shared_from_this());
  asio::async_write(socket_,
                    asio::buffer(buf->read_pos(), buf->readable_size()),
                    [this, self](const ErrorCode &ec, std::size_t len) {
                      SNIPPET_ENTER_CB;
                      send_buffers_.pop();
                      if (!send_buffers_.empty()) { do_send(); }
                    });
}

void HttpFlvSub::close() {
  socket_.close();
  if (close_cb_) { close_cb_(shared_from_this()); }
}

}
