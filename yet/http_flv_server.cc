#include "http_flv_server.h"
#include "http_flv_sub.h"
#include "http_flv_pull.h"
#include "yet_group.h"
#include "yet_inner.hpp"

namespace yet {

HttpFlvServer::HttpFlvServer(const std::string &listen_ip, uint16_t listen_port)
  : listen_ip_(listen_ip)
  , listen_port_(listen_port)
  , acceptor_(io_ctx_, asio::ip::tcp::endpoint(asio::ip::address_v4::from_string(listen_ip_), listen_port_))
{
}

void HttpFlvServer::do_accept() {
  acceptor_.async_accept(std::bind(&HttpFlvServer::accept_cb, shared_from_this(), _1, _2));
}

void HttpFlvServer::accept_cb(const ErrorCode &ec, asio::ip::tcp::socket socket) {
  YET_LOG_DEBUG("accept_cb ec:{}", ec.message());

  if (!ec) {
    auto pull = std::make_shared<HttpFlvSub>(std::move(socket), shared_from_this());
    pull->start();

  } else {
    YET_LOG_ERROR("Accept failed. ec:{}", ec.message());
  }

  do_accept();
}

void HttpFlvServer::run_loop() {
  do_accept();

  io_ctx_.run();
}

void HttpFlvServer::on_http_flv_request(HttpFlvSubPtr sub, const std::string &uri, const std::string &app_name,
                                        const std::string &live_name, const std::string &host)
{
  GroupPtr group;
  auto lng_iter = live_name_2_group_.find(live_name);
  if (lng_iter != live_name_2_group_.end()) {
    YET_LOG_DEBUG("on_http_flv_request. {} group exist.", live_name);
    group = lng_iter->second;
  } else {
    group = std::make_shared<Group>(live_name);
    live_name_2_group_[live_name] = group;
    YET_LOG_DEBUG("on_http_flv_request. {} create group.", live_name);
  }
  auto in = group->get_in();
  if (in) {
    YET_LOG_DEBUG("on_http_flv_request. {} in exist.", live_name);
  } else {
    YET_LOG_DEBUG("on_http_flv_request. {} create in.", live_name);
    in = std::make_shared<HttpFlvPull>(io_ctx_, config.http_flv_pull_host(), uri);
    group->set_http_flv_pull(in);
  }

  group->add_out(sub);
}

}
