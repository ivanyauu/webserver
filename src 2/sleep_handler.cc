#include <string>
#include <memory>
#include <boost/beast/http.hpp>
#include <thread>
#include <chrono>
#include "sleep_handler.h"
namespace http = boost::beast::http;

std::unique_ptr<request_handler> sleep_handler::init(std::string root) {
    return std::make_unique<sleep_handler>();
}

sleep_handler::sleep_handler() {}

http::response<http::string_body> sleep_handler::handle_request(http::request<http::string_body> request) {
    http::response<http::string_body> response;
    response.version(11);
    response.result(http::status::ok);
    response.set(http::field::content_type, "text/plain");
    response.prepare_payload();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return response;
}
