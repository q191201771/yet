/**
 * @file   yet_http_flv_pull.h
 * @author pengrl
 *
 */

#pragma once

#include <asio.hpp>
#include "yet.hpp"
#include "yet_http_flv/yet_http_flv_buffer_t.hpp"

namespace yet {

class HttpFlvPull : public std::enable_shared_from_this<HttpFlvPull> {
  public:
    HttpFlvPull(asio::io_context& io_context, const std::string& server, const std::string& path);
    ~HttpFlvPull();

    void set_group(std::weak_ptr<Group> group);

    void dispose() {}

    BufferPtr get_metadata();
    BufferPtr get_avc_header();
    BufferPtr get_aac_header();

  private:
    void resolve_cb(const ErrorCode &ec, const asio::ip::tcp::resolver::results_type &endpoints);
    void connect_cb(const ErrorCode &ec);
    void write_request_cb(const ErrorCode &ec);
    void do_read_header_stuff();
    void read_header_stuff_cb(const ErrorCode &ec, std::size_t len);
    void do_read_flv_body();
    void read_flv_body_cb(const ErrorCode &ec, std::size_t len);

  private:
    void flv_body_handler();

  private:
    HttpFlvPull(const HttpFlvPull &) = delete;
    HttpFlvPull &operator=(const HttpFlvPull &) = delete;

  private:
    enum Stage {
      STAGE_HTTP_STATUS_LINE = 0,
      STAGE_HTTP_HEADERS = 1,
      STAGE_FLV_HEADER = 2,
      STAGE_FLV_BODY = 3
    };

    enum Substage {
      SUBSTAGE_TAG_HEADER = 0,
      SUBSTAGE_ENSURE_PREFIX_OF_DATA = 1,
      SUBSTAGE_TAG_DATA = 2
    };

    asio::ip::tcp::resolver resolver_;
    asio::ip::tcp::socket   socket_;
    std::weak_ptr<Group>    group_;
    BufferPtr               in_buf_;
    TagCacheMetadata        cache_meta_data_;
    TagCacheAvcHeader       cache_avc_header_;
    TagCacheAacHeader       cache_aac_header_;
    asio::streambuf         request_;
    asio::streambuf         response_;
    enum Stage              stage_ = STAGE_HTTP_STATUS_LINE;
    enum Substage           substage_ = SUBSTAGE_TAG_HEADER;
    std::size_t             tag_data_size_ = 0;
};

}
