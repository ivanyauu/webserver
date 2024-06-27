#ifndef STATIC_HANDLER_H
#define STATIC_HANDLER_H

#include <string>
#include "request_handler.h"

class static_handler: public request_handler {
public:
    static std::unique_ptr<request_handler> init(std::string root);

    // Constructor.
    static_handler(std::string root);

    // Takes in an HTTP request for a static file and returns it if it exists.
    http::response<http::string_body> handle_request(http::request<http::string_body> request) override;

private:
    std::string root_;
};

#endif
