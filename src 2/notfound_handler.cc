#include <string>
#include <memory>
#include <boost/beast/http.hpp>
#include <boost/lexical_cast.hpp>
#include "notfound_handler.h"
namespace http = boost::beast::http;

std::unique_ptr<request_handler> notfound_handler::init(std::string root) {
    return std::make_unique<notfound_handler>();
}

notfound_handler::notfound_handler() {}

http::response<http::string_body> notfound_handler::handle_request(http::request<http::string_body> request) {
    http::response<http::string_body> response;
    response.version(11);
    response.set(http::field::content_type, "text/html");
    response.result(http::status::not_found);
    response.body() = "<html><head><title>Not Found</title></head><body><h1>404 Not Found</h1></body></html>";
    response.prepare_payload();
    return response;
}