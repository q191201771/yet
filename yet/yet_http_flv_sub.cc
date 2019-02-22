#include "yet_http_flv_sub.h"
#include <string>
#include <asio.hpp>
#include "yet_http_flv_pull.h"
#include "yet_group.h"
#include "yet.hpp"
#include "chef_base/chef_strings_op.hpp"
#include "chef_base/chef_stuff_op.hpp"

#define SNIPPET_HANDLE_CB_ERROR if (ec) { YET_LOG_ERROR("ec:{}", ec.message()); return; }

namespace yet {

HttpFlvSub::HttpFlvSub(asio::ip::tcp::socket socket)
  : socket_(std::move(socket))
  , request_buf_(1024)
{
  YET_LOG_DEBUG("HttpFlvSub() {}.", static_cast<void *>(this));
}

HttpFlvSub::~HttpFlvSub() {
  YET_LOG_DEBUG("~HttpFlvSub() {}.", static_cast<void *>(this));
}

void HttpFlvSub::set_sub_cb(HttpFlvSubCb cb) { sub_cb_ = cb; }

void HttpFlvSub::set_close_cb(HttpFlvSubEventCb cb) { close_cb_ = cb; }

void HttpFlvSub::start() {
  asio::async_read_until(socket_, request_buf_, "\r\n\r\n", std::bind(&HttpFlvSub::request_handler, shared_from_this(), _1, _2));
}

void HttpFlvSub::request_handler(const ErrorCode &ec, std::size_t len) {
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

  if (sub_cb_) { sub_cb_(shared_from_this(), uri, app_name, live_name, host); }

  do_send_http_headers();
}

void HttpFlvSub::do_send_http_headers() {
  asio::async_write(socket_, asio::buffer(FLV_HTTP_HEADERS, FLV_HTTP_HEADERS_LEN),
                    std::bind(&HttpFlvSub::send_http_headers_cb, shared_from_this(), _1));
}

void HttpFlvSub::send_http_headers_cb(const ErrorCode &ec) {
  SNIPPET_HANDLE_CB_ERROR;
  do_send_flv_header();
}


void HttpFlvSub::do_send_flv_header() {
  asio::async_write(socket_, asio::buffer(FLV_HEADER_BUF_13, 13),
                    std::bind(&HttpFlvSub::send_flv_header_cb, shared_from_this(), _1));
}

void HttpFlvSub::send_flv_header_cb(const ErrorCode &ec) {
  SNIPPET_HANDLE_CB_ERROR;

  if (auto group = group_.lock()) {
    auto metadata = group->get_metadata();
    if (!metadata) {
      YET_LOG_DEBUG("cache metadata not exist.");
      is_bc_ready_ = true;
    } else {
      YET_LOG_DEBUG("{}", metadata->readable_size());
      asio::async_write(socket_, asio::buffer(metadata->read_pos(), metadata->readable_size()),
                        std::bind(&HttpFlvSub::send_metadata_cb, shared_from_this(), _1));
    }
  }
}

void HttpFlvSub::send_metadata_cb(const ErrorCode &ec) {
  SNIPPET_HANDLE_CB_ERROR;

  YET_LOG_INFO("Sent cached metadata.");

  if (auto group = group_.lock()) {
    auto header = group->get_avc_header();
    if (!header) {
      is_bc_ready_ = true;
    } else {
      asio::async_write(socket_, asio::buffer(header->read_pos(), header->readable_size()),
                        std::bind(&HttpFlvSub::send_avc_header_cb, shared_from_this(), _1));
    }
  }
}

void HttpFlvSub::send_avc_header_cb(const ErrorCode &ec) {
  SNIPPET_HANDLE_CB_ERROR;

  YET_LOG_INFO("Sent cached avc header.");

  if (auto group = group_.lock()) {
    auto header = group->get_aac_header();
    if (!header) {
      YET_LOG_DEBUG("CHEFERASEME");
      is_bc_ready_ = true;
    } else {
      asio::async_write(socket_, asio::buffer(header->read_pos(), header->readable_size()),
                        std::bind(&HttpFlvSub::send_aac_header_cb, shared_from_this(), _1));
    }
  }
}

void HttpFlvSub::send_aac_header_cb(const ErrorCode &ec) {
  SNIPPET_HANDLE_CB_ERROR;

  YET_LOG_INFO("Sent cached aac header.");

  is_bc_ready_ = true;
}

void HttpFlvSub::async_send(BufferPtr buf, const std::vector<FlvTagInfo> &tis) {
  if (!is_bc_ready_) { return; }

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

  bool is_empty = send_buffers_.empty();
  send_buffers_.push(buf);
  if (is_empty && is_bc_ready_) {
    do_send();
  }
}

void HttpFlvSub::do_send() {
  BufferPtr buf = send_buffers_.front();
  asio::async_write(socket_, asio::buffer(buf->read_pos(), buf->readable_size()),
                    std::bind(&HttpFlvSub::send_cb, shared_from_this(), _1, _2));
}

void HttpFlvSub::send_cb(const ErrorCode &ec, std::size_t len) {
  //SNIPPET_HANDLE_CB_ERROR;
  if (ec) {
    YET_LOG_ERROR("ec:{}", ec.message());
    if (ec.value() == asio::error::broken_pipe) {
      close();
    }
    return;
  }

  send_buffers_.pop();
  if (!send_buffers_.empty()) {
    do_send();
  }
}

void HttpFlvSub::set_group(std::weak_ptr<Group> group) {
  group_ = group;
}

void HttpFlvSub::close() {
  socket_.close();
  if (close_cb_) {
    close_cb_(shared_from_this());
  }
  //if (auto group = group_.lock()) {
  //  group->del_http_flv_sub(shared_from_this());
  //}
}

}
