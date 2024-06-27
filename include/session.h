#ifndef SESSION_HPP
#define SESSION_HPP

#include <boost/asio.hpp>
#include <string>
#include <vector>
#include "router.h"
#include "config_parser.h"

using boost::asio::ip::tcp;
namespace http = boost::beast::http;

class session {
public:
  session(boost::asio::io_service& io_service, std::vector<HandlerConfig>& handlers);
  tcp::socket& socket();
  void start();
  void handle_read(const boost::system::error_code& error, size_t bytes_transferred);
  void handle_write(const boost::system::error_code& error);
  std::string process_request(http::request<http::string_body>& parsed);
private:
  void do_read();
  void write_response(const boost::system::error_code& error, std::string response);
  void read_request(const boost::system::error_code& error);
  tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];
  router router_;
  boost::optional<http::request_parser<http::string_body>> parser_;
  std::string response_metric = "[ResponseMetrics]";
};

#endif
