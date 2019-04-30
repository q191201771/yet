#include "yet_http_flv_server.h"
#include <asio.hpp>
#include "yet.hpp"
#include "yet_config.h"
#include "yet_server.h"
#include "yet_group.h"
#include "yet_http_flv_session_sub.h"

namespace yet {

HttpFlvServer::HttpFlvServer(asio::io_context &io_ctx, const std::string &listen_ip, uint16_t listen_port, Server *server)
  : io_ctx_(io_ctx)
  , listen_ip_(listen_ip)
  , listen_port_(listen_port)
  , server_(server)
{
  YET_LOG_DEBUG("[{}] [lifecycle] new HttpFlvServer.", (void *)this);
}

HttpFlvServer::~HttpFlvServer() {
  YET_LOG_DEBUG("[{}] [lifecycle] delete HttpFlvServer.", (void *)this);
}

void HttpFlvServer::do_accept() {
  acceptor_->async_accept([this](const ErrorCode &ec, asio::ip::tcp::socket socket) {
                          YET_LOG_ASSERT(!ec, "http flv server accept failed. ec:{}", ec.message());
                          auto pull = std::make_shared<HttpFlvSub>(std::move(socket));
                          pull->set_sub_cb([this](auto ...args) { on_http_flv_request(std::forward<decltype(args)>(args)...); });
                          pull->start();
                          do_accept();
                        });
}

bool HttpFlvServer::start() {
  YET_LOG_INFO("start http flv server. {}:{}", listen_ip_, listen_port_);
  try {
    acceptor_ = std::make_unique<asio::ip::tcp::acceptor>(io_ctx_,
        asio::ip::tcp::endpoint(asio::ip::address_v4::from_string(listen_ip_), listen_port_));
  } catch (const std::system_error &err) {
    YET_LOG_ERROR("http flv server listen failed. ip:{}, port:{}, ec:{}", listen_ip_, listen_port_, err.code().message());
    return false;
  }
  do_accept();
  return true;
}

void HttpFlvServer::dispose() {
  YET_LOG_INFO("dispose http flv server.");
  acceptor_->close();
}

void HttpFlvServer::on_http_flv_request(HttpFlvSubPtr sub, const std::string &uri, const std::string &app_name,
                                        const std::string &stream_name, const std::string &host)
{
  (void)uri;(void)app_name;(void)host;
  auto group = server_->get_or_create_group(app_name, stream_name);

  group->add_http_flv_sub(sub);
}

}
