#ifndef HEALTH_HANDLER_H
#define HEALTH_HANDLER_H

#include <string>
#include <memory>
#include "request_handler.h"

class health_handler : public request_handler {
public:
    static std::unique_ptr<request_handler> init(std::string root);

    // Constructor
    health_handler();

    http::response<http::string_body> handle_request(http::request<http::string_body> request) override;
};

#endif
