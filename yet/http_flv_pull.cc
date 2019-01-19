#include "http_flv_pull.h"
#include "yet_inner.hpp"
#include "yet_group.h"
#include <chef_base/chef_strings_op.hpp>
#include <chef_base/chef_stuff_op.hpp>
#include <chef_base/chef_stringify_stl.hpp>

#define SNIPPET_HANDLE_CB_ERROR if (ec) { YET_LOG_ERROR("ec:{}", ec.message()); return; }

namespace yet {

static constexpr std::size_t INIT_IN_BUFFER_LEN         = 16384;

HttpFlvPull::HttpFlvPull(asio::io_context &io_context, const std::string &server, const std::string &path)
  : resolver_(io_context)
  , socket_(io_context)
  , in_buf_(std::make_shared<chef::buffer>(INIT_IN_BUFFER_LEN))
{
  std::ostream request_stream(&request_);
  request_stream << "GET " << path << " HTTP/1.0\r\n";
  //request_stream << "User-Agent: yet/0.0.1" << path << " HTTP/1.0\r\n";
  request_stream << "Accept: */*\r\n";
  request_stream << "Range: byte=0-\r\n";
  request_stream << "Connection: close\r\n";
  request_stream << "Host: " << server << "\r\n";
  request_stream << "Icy-MetaData: 1\r\n\r\n";

  resolver_.async_resolve(server, "http", std::bind(&HttpFlvPull::resolve_cb, this, _1, _2));
}

void HttpFlvPull::resolve_cb(const ErrorCode &ec, const asio::ip::tcp::resolver::results_type &endpoints) {
  SNIPPET_HANDLE_CB_ERROR;
  asio::async_connect(socket_, endpoints, std::bind(&HttpFlvPull::connect_cb, shared_from_this(), _1));
}

void HttpFlvPull::connect_cb(const ErrorCode &ec) {
  SNIPPET_HANDLE_CB_ERROR;
  asio::async_write(socket_, request_, std::bind(&HttpFlvPull::write_request_cb, shared_from_this(), _1));
}

void HttpFlvPull::write_request_cb(const ErrorCode &ec) {
  SNIPPET_HANDLE_CB_ERROR;
  do_read_header_stuff();
}

void HttpFlvPull::do_read_header_stuff() {
  socket_.async_read_some(asio::buffer(in_buf_->write_pos(), INIT_IN_BUFFER_LEN),
                          std::bind(&HttpFlvPull::read_header_stuff_cb, shared_from_this(), _1, _2));
}

void HttpFlvPull::read_header_stuff_cb(const ErrorCode &ec, std::size_t len) {
  SNIPPET_HANDLE_CB_ERROR;

  in_buf_->seek_write_pos(len);
  YET_LOG_DEBUG("len:{} {}", len, in_buf_->readable_size());

  for (; in_buf_->readable_size(); ) {
    if (stage_ == STAGE_HTTP_STATUS_LINE) {
      uint8_t *q = in_buf_->find_crlf();
      if (q) {
        uint8_t *p = in_buf_->read_pos();
        std::string status_line = std::string((const char *)p, q-p);
        YET_LOG_DEBUG("{}", status_line);

        auto sls = chef::strings_op::split(status_line, " ");
        if (sls.size() != 3 ||
            !chef::strings_op::has_prefix(sls[0], "HTTP/") || sls[1] != "200" || sls[2] != "OK")
        {
          YET_LOG_ERROR("Invalid status line, {}", status_line);
          return;
        }

        in_buf_->erase(q-p+2);
        stage_ = STAGE_HTTP_HEADERS;
      }
    } else if (stage_ == STAGE_HTTP_HEADERS) {
      uint8_t *q = in_buf_->find((const uint8_t *)"\r\n\r\n", 4);
      if (q) {
        uint8_t *p = in_buf_->read_pos();
        std::string headers = std::string((const char *)p, q-p);
        YET_LOG_DEBUG("{}", headers);
        in_buf_->erase(q-p+4);
        stage_ = STAGE_FLV_HEADER;

        if (auto group = group_.lock()) {
          group->on_connected();
        }
      }
    } else if (stage_ == STAGE_FLV_HEADER) {
      if (in_buf_->readable_size() < 13) { return; }

      uint8_t *p = in_buf_->read_pos();
      if (*p != 'F' || *(p+1) != 'L' || *(p+2) != 'V' ||
          chef::stuff_op::read_be_int(p+5, 4) != FLV_HEADER_FLV_VERSION
      ) {
        YET_LOG_ERROR("Invalid flv header. {}", chef::stuff_op::bytes_to_hex(in_buf_->read_pos(), 32, 16));
        return;
      }

      in_buf_->erase(13);
      stage_ = STAGE_FLV_BODY;
    } else if (stage_ == STAGE_FLV_BODY) {
      flv_body_handler();
      return;
    }
  }

  do_read_header_stuff();
}

void HttpFlvPull::do_read_flv_body() {
  std::size_t len = INIT_IN_BUFFER_LEN - in_buf_->readable_size();
  in_buf_->reserve(len);
  socket_.async_read_some(asio::buffer(in_buf_->write_pos(), len), std::bind(&HttpFlvPull::read_flv_body_cb, shared_from_this(), _1, _2));
}

void HttpFlvPull::read_flv_body_cb(const ErrorCode &ec, std::size_t len) {
  SNIPPET_HANDLE_CB_ERROR;
  //YET_LOG_DEBUG("{}", len);
  in_buf_->seek_write_pos(len);
  flv_body_handler();
}

void HttpFlvPull::flv_body_handler() {
  static constexpr std::size_t ENSURE_PREFIX_OF_AUDIO_DATA = 3;
  static constexpr std::size_t ENSURE_PREFIX_OF_VIDEO_DATA = 5;

  std::vector<FlvTagInfo> tis;

  std::size_t rs = in_buf_->readable_size();
  uint8_t *p = in_buf_->read_pos();
  for (; rs; ) {
    if (substage_ == SUBSTAGE_TAG_HEADER) {
      if (rs < FLV_TAG_HEADER_LEN) { break; }

      uint8_t tag_type = *p;

      if (tag_type == FLVTAGTYPE_VIDEO && (rs < FLV_TAG_HEADER_LEN + ENSURE_PREFIX_OF_VIDEO_DATA)) {
        YET_LOG_INFO("CHEFNOTICEME.");
        break;
      }
      if (tag_type == FLVTAGTYPE_AUDIO && (rs < FLV_TAG_HEADER_LEN + ENSURE_PREFIX_OF_AUDIO_DATA)) {
        YET_LOG_INFO("CHEFNOTICEME.");
        break;
      }

      FlvTagInfo ti;
      ti.tag_pos = p;
      ti.tag_type = static_cast<FlvTagType>(tag_type);
      tag_data_size_ = chef::stuff_op::read_be_int(p+1, 3);
      ti.tag_whole_size = tag_data_size_ + FLV_TAG_HEADER_LEN + 4;

      tis.push_back(ti);
      rs -= FLV_TAG_HEADER_LEN;
      p += FLV_TAG_HEADER_LEN;
      substage_ = SUBSTAGE_TAG_DATA;
    } else if (substage_ == SUBSTAGE_TAG_DATA) {
      if (rs < (tag_data_size_ + 4)) {
        tag_data_size_ -= rs;
        rs = 0;
        p += rs;
        break;
      }

      rs -= (tag_data_size_ + 4);
      p += tag_data_size_ + 4;
      substage_ = SUBSTAGE_TAG_HEADER;
    }
  }

  BufferPtr ref_buffer = in_buf_;
  in_buf_ = std::make_shared<Buffer>(INIT_IN_BUFFER_LEN);

  if (substage_ == SUBSTAGE_TAG_HEADER && rs > 0) {
    YET_LOG_ASSERT(rs < FLV_TAG_HEADER_LEN + ENSURE_PREFIX_OF_VIDEO_DATA, "{}", rs);
    in_buf_->append(ref_buffer->write_pos()-rs, rs);
    ref_buffer->seek_write_pos_rollback(rs);
  }

  //YET_LOG_DEBUG("on_data {} {}", chef::stringify_stl(tis), ref_buffer->readable_size());
  if (ref_buffer->readable_size()) {
    tcmd_.on_buffer(ref_buffer, tis);
    tcvsh_.on_buffer(ref_buffer, tis);

    if (auto group = group_.lock()) {
      group->on_data(ref_buffer, tis);
    }
  }

  do_read_flv_body();
}

BufferPtr HttpFlvPull::get_metadata() {
  return tcmd_.buf();
}

BufferPtr HttpFlvPull::get_video_seq_header() {
  return tcvsh_.buf();
}

void HttpFlvPull::set_group(std::weak_ptr<Group> group) {
  group_ = group;
}

} // namespace yet

