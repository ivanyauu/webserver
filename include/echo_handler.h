#ifndef ECHO_HANDLER_H
#define ECHO_HANDLER_H

// Defines a request handler which echos back the request

#include <string>
#include <memory>
#include "request_handler.h"

class echo_handler : public request_handler {
public:
    static std::unique_ptr<request_handler> init(std::string root);

    // Constructor
    echo_handler();

    // Takes an HTTP request and echos the request.
    http::response<http::string_body> handle_request(http::request<http::string_body> request) override;
};

#endif // ECHO_HANDLER_H
