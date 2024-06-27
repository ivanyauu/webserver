#include "session.h"
#include "logger.h"
#include "router.h"
#include "request_handler.h"
#include "config_parser.h"
#include <boost/bind.hpp>
#include <boost/beast/http.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <memory>
#include <vector>
#include <string>

namespace http = boost::beast::http;
Logger *logger = Logger::get_global_log();

session::session(boost::asio::io_service& io_service, std::vector<HandlerConfig>& handlers)
  : socket_(io_service),
  router_(handlers)
{
}

tcp::socket& session::socket() {
  return socket_;
}

void session::start() {
  do_read();
}

void session::do_read() {
  socket_.async_read_some(boost::asio::buffer(data_, max_length),
      boost::bind(&session::handle_read, this,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
}

void session::handle_read(const boost::system::error_code& error, size_t bytes_transferred) {
  if (!error) {
    logger->logDebug("Reading the Handler of Request");
    std::string request = data_;

    logger->logDebug("Request packet: " + request);

    if (!parser_) {
      parser_.emplace();
    }

    boost::beast::error_code ec;
    parser_->eager(true);
    parser_->put(boost::asio::buffer(request), ec);
    if (ec) {
      http::response<http::string_body> response;
      response.version(11);
      response.result(http::status::bad_request);
      write_response(error, boost::lexical_cast<std::string>(response));
      return;
    }

    if (parser_->is_done()) {
      http::request<http::string_body> parsed = parser_->release();
      std::string response = process_request(parsed);
      write_response(error, response);

      // reset the parser
      parser_.emplace();
    } else {
      logger->logDebug("Request not complete, continue reading");
      do_read();
    }
  } else {
    logger->logError("ERROR: Reading request");
    delete this;
  }
}

std::string session::process_request(http::request<http::string_body>& parsed) {
  logger->logDebug("Processing the Request");
  std::string log_handler_name = "";

  http::response<http::string_body> response;
  response.version(11);

  std::string path = std::string(parsed.target());

  std::unique_ptr<request_handler> handler = router_.match(path, log_handler_name);
  if (handler == nullptr) {
    response.result(http::status::internal_server_error);
    logger->logResponseMetric(parsed, response, log_handler_name, response_metric);
    return boost::lexical_cast<std::string>(response);
  }

  response = handler->handle_request(parsed);
  logger->logResponseMetric(parsed, response, log_handler_name, response_metric);


  return boost::lexical_cast<std::string>(response);
}


void session::handle_write(const boost::system::error_code& error) {
  if (!error) {
    read_request(error);
  } else {
    logger->logError("ERROR: Writing response");
    delete this;
  }
}

void session::write_response(const boost::system::error_code& error, std::string response) {
  boost::asio::async_write(socket_,
			   boost::asio::buffer(response, response.size()),
			   boost::bind(&session::handle_write, this,
				       boost::asio::placeholders::error));
  logger->logDebug("Response: " + response);
}

void session::read_request(const boost::system::error_code& error) {
  socket_.async_read_some(boost::asio::buffer(data_, max_length),
        boost::bind(&session::handle_read, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
}
