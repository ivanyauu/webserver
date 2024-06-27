#ifndef LOGGER_H
#define LOGGER_H

#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/signals2.hpp>
#include <boost/beast/http.hpp>

namespace http = boost::beast::http;
using boost::asio::ip::tcp;


class Logger {
public:
  Logger();
  ~Logger();
  static Logger* logger;
  void logTrace(std::string message);
  void logDebug(std::string message);
  void logInfo(std::string message);
  void logFatal(std::string message);
  void logError(std::string message);
  void logWarning(std::string message);
  void logResponseMetric(http::request<http::string_body>& request, http::response<http::string_body>& response, std::string log_handler_name, std::string response_metric);
  void init_logging();
  static Logger * get_global_log();
};

#endif
