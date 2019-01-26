/**
 * @file   http_flv.hpp
 * @author pengrl
 *
 */

#pragma once

// config
namespace yet {

static constexpr char FLV_HTTP_HEADERS[] = \
  "HTTP/1.1 200 OK\r\n" \
  "Cache-Control: no-cache\r\n" \
  "Content-Type: video/x-flv\r\n" \
  "Connection: close\r\n" \
  "Expires: -1\r\n" \
  "Pragma: no-cache\r\n" \
  "\r\n"
  ;
static constexpr std::size_t FLV_HTTP_HEADERS_LEN = sizeof(FLV_HTTP_HEADERS)-1;

}

// const
namespace yet {

static constexpr std::size_t FLV_HEADER_FLV_VERSION = 9;

static constexpr uint8_t FLV_HEADER_BUF_13[] = { 0x46, 0x4c, 0x56, 0x01, 0x05, 0x0, 0x0, 0x0, 0x09, 0x0, 0x0, 0x0, 0x0 };

static constexpr std::size_t FLV_TAG_HEADER_LEN = 11;

static constexpr std::size_t FLV_TAG_HEADER_TYPE_AUDIO       = 8;
static constexpr std::size_t FLV_TAG_HEADER_TYPE_VIDEO       = 9;
static constexpr std::size_t FLV_TAG_HEADER_TYPE_SCRIPT_DATA = 18;

}
