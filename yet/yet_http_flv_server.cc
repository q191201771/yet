#include "yet_http_flv_server.h"
#include <asio.hpp>
#include "yet_config.h"
#include "yet_server.h"
#include "yet_group.h"
#include "yet_http_flv_sub.h"
#include "yet_http_flv_pull.h"
#include "yet.hpp"

namespace yet {

HttpFlvServer::HttpFlvServer(asio::io_context &io_ctx, const std::string &listen_ip, uint16_t listen_port, Server *server)
  : io_ctx_(io_ctx)
  , listen_ip_(listen_ip)
  , listen_port_(listen_port)
  , server_(server)
  , acceptor_(io_ctx_, asio::ip::tcp::endpoint(asio::ip::address_v4::from_string(listen_ip_), listen_port_))
{
  YET_LOG_DEBUG("HttpFlvServer(). {}", (void *)this);
}

HttpFlvServer::~HttpFlvServer() {
  YET_LOG_DEBUG("~HttpFlvServer(). {}", (void *)this);
}

void HttpFlvServer::do_accept() {
  acceptor_.async_accept(std::bind(&HttpFlvServer::accept_cb, shared_from_this(), _1, _2));
}

void HttpFlvServer::accept_cb(const ErrorCode &ec, asio::ip::tcp::socket socket) {
  if (ec) {
    YET_LOG_ERROR("http flv server accept failed. ec:{}", ec.message());
    return;
  }

  auto pull = std::make_shared<HttpFlvSub>(std::move(socket));
  pull->set_sub_cb(std::bind(&HttpFlvServer::on_http_flv_request, this, _1, _2, _3, _4, _5));
  pull->start();

  do_accept();
}

void HttpFlvServer::start() {
  YET_LOG_INFO("start http flv server. {}:{}", listen_ip_, listen_port_);
  do_accept();
}

void HttpFlvServer::dispose() {
  YET_LOG_INFO("dispose http flv server.");
  acceptor_.close();
}

void HttpFlvServer::on_http_flv_request(HttpFlvSubPtr sub, const std::string &uri, const std::string &app_name,
                                        const std::string &live_name, const std::string &host)
{
  auto group = server_->get_or_create_group(live_name);

  auto in = group->get_http_flv_pull();
  if (in) {
    YET_LOG_DEBUG("on_http_flv_request. {} in exist.", live_name);
  } else {
    YET_LOG_DEBUG("on_http_flv_request. {} create in.", live_name);
    in = std::make_shared<HttpFlvPull>(io_ctx_, Config::instance()->http_flv_pull_host(), uri);
    group->set_http_flv_pull(in);
  }

  group->add_http_flv_sub(sub);
}

}
