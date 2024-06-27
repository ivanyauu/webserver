#include <string>
#include <memory>
#include <boost/beast/http.hpp>
#include <boost/lexical_cast.hpp>
#include "echo_handler.h"
namespace http = boost::beast::http;

std::unique_ptr<request_handler> echo_handler::init(std::string root) {
    return std::make_unique<echo_handler>();
}

echo_handler::echo_handler() {}

http::response<http::string_body> echo_handler::handle_request(http::request<http::string_body> request) {
    http::response<http::string_body> response;
    response.version(11);
    response.set(http::field::content_type, "text/plain");
    response.body() = boost::lexical_cast<std::string>(request);
    response.prepare_payload();
    return response;
}
