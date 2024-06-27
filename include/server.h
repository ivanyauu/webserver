#ifndef SERVER_H
#define SERVER_H

#include <boost/asio.hpp>
#include <vector>
#include <string>
#include "session.h"
#include "request_handler.h"

using boost::asio::ip::tcp;

class server {
public:
  server(boost::asio::io_service& io_service, short port, std::vector<HandlerConfig>& handlers);
private:
  void start_accept();
  void handle_accept(session* new_session, const boost::system::error_code& error);
  boost::asio::io_service& io_service_;
  tcp::acceptor acceptor_;
  std::vector<HandlerConfig> handlers_;
};

#endif
