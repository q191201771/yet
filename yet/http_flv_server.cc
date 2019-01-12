#include "http_flv_server.h"
#include "http_flv_session.h"
#include "http_flv_in.h"
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

void HttpFlvServer::accept_cb(ErrorCode ec, asio::ip::tcp::socket socket) {
  YET_LOG_DEBUG("accept_cb ec:{}", ec.message());

  if (!ec) {
    auto session = std::make_shared<HttpFlvSession>(std::move(socket), shared_from_this());
    session->start();

  } else {
    YET_LOG_ERROR("Accept failed. ec:{}", ec.message());
  }

  do_accept();
}

void HttpFlvServer::run_loop() {
  do_accept();

  io_ctx_.run();
}

void HttpFlvServer::on_request(HttpFlvSessionPtr hfs, const std::string &uri, const std::string &app_name,
                               const std::string &live_name, const std::string &host)
{
  auto group = std::make_shared<Group>(live_name);
  live_name_2_group_[live_name] = group;
  auto in = std::make_shared<HttpFlvIn>(io_ctx_, host, uri);
  group->set_http_flv_in(in);
  group->add_out(hfs);
}

}
