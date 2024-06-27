#include <string>
#include <memory>
#include <boost/beast/http.hpp>
#include "health_handler.h"
namespace http = boost::beast::http;

std::unique_ptr<request_handler> health_handler::init(std::string root) {
    return std::make_unique<health_handler>();
}

health_handler::health_handler() {}

http::response<http::string_body> health_handler::handle_request(http::request<http::string_body> request) {
    http::response<http::string_body> response;
    response.version(11);
    response.set(http::field::content_type, "text/plain");
    response.result(http::status::ok);
    response.body() = "OK";
    response.prepare_payload();
    return response;
}