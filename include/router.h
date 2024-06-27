#ifndef ROUTER_H
#define ROUTER_H

#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include "request_handler.h"
#include "config_parser.h"

class router {
    public:
        router(std::vector<HandlerConfig>& handlers);
        std::unique_ptr<request_handler> match(const std::string& path, std::string& log_handler_name);

    private:
        std::vector<HandlerConfig> handlers_;
        static std::unordered_map<std::string, request_handler_factory> handler_registry_;
        
};

#endif // ROUTER_H
