#include "logger.h"
#include <string>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;
namespace sinks = boost::log::sinks;

using namespace logging::trivial;

Logger::Logger() {
  init_logging();
}

Logger::~Logger() {
  if (Logger::logger != 0) {
    delete Logger::logger;
    Logger::logger = 0;
  }
}

void Logger::logTrace(std::string message){
  BOOST_LOG_TRIVIAL(trace) << message;
}

void Logger::logDebug(std::string message){
  BOOST_LOG_TRIVIAL(debug) << message;
}

void Logger::logInfo(std::string message){
  BOOST_LOG_TRIVIAL(info) << message;
}

void Logger::logFatal(std::string message){
  BOOST_LOG_TRIVIAL(fatal) << message;
}

void Logger::logError(std::string message){
  BOOST_LOG_TRIVIAL(error) << message;
}

void Logger::logWarning(std::string message){
  BOOST_LOG_TRIVIAL(warning) << message;
}

void Logger::logResponseMetric(http::request<http::string_body>& request, http::response<http::string_body>& response, std::string log_handler_name, std::string response_metric){
    std::string path = std::string(request.target());
    std::string response_status = std::to_string(response.result_int());
    std::string handler = log_handler_name;

    if (log_handler_name == "")
    {
      handler = "No Matching Handler";
    }

    std::stringstream stream;
    stream << response_metric << " ";
    stream << "response_code:" << response_status << " ";
    stream << "Handler: " << handler << " ";
    stream << "Path:" << " " << path << " ";
    
    BOOST_LOG_TRIVIAL(trace) << stream.str();
}

void Logger::init_logging() {
  logging::add_file_log(keywords::auto_flush = true,
			keywords::file_name = "../src/logfiles/my_log_%Y-%m-%d.log",
			keywords::rotation_size = 10 * 1024 * 1024,
			keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
			keywords::format = "[%TimeStamp%] [%ThreadID%] [%Severity%] %Message%"
			);
  logging::add_console_log(std::cout, boost::log::keywords::format = ">> %Message%");
  logging::add_common_attributes();
  logging::register_simple_formatter_factory<logging::trivial::severity_level, char>("Severity");
}

Logger * Logger::get_global_log(){
  if (Logger::logger == 0){
    Logger::logger = new Logger();
  }
  return Logger::logger;
}

Logger* Logger::logger = 0;
