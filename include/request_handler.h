#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

// Base class defining an HTTP request handler.
#include <string>
#include <functional>
#include <boost/beast/http.hpp>
namespace http = boost::beast::http;

class request_handler {
public:
    // Takes an HTTP request and returns and HTTP response.
    virtual http::response<http::string_body> handle_request(http::request<http::string_body> request) = 0;
};

// Function that creates a request handler given a root string.
using request_handler_factory = std::function<std::unique_ptr<request_handler>(std::string)>;


#endif // REQUEST_HANDLER_H
