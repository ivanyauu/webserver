#ifndef NOTFOUND_HANDLER_H
#define NOTFOUND_HANDLER_H

#include <string>
#include <memory>
#include "request_handler.h"

class notfound_handler : public request_handler {
public:
    static std::unique_ptr<request_handler> init(std::string root);

    // Constructor
    notfound_handler();

    // Takes an HTTP request and echos the request.
    http::response<http::string_body> handle_request(http::request<http::string_body> request) override;
};

#endif  // NOTFOUND_HANDLE