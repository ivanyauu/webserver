#include "config_parser.h"
#include "router.h"
#include "echo_handler.h"
#include "static_handler.h"
#include "notfound_handler.h"
#include "crud_handler.h"
#include "sleep_handler.h"
#include "health_handler.h"
#include "markdown_handler.h"
#include "request_handler.h"
#include "logger.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <iostream>

std::unordered_map<std::string, request_handler_factory> router::handler_registry_ = {
  {"echo_handler", echo_handler::init},
  {"static_handler", static_handler::init},
  {"notfound_handler", notfound_handler::init},
  {"crud_handler", crud_handler::init},
  {"sleep_handler", sleep_handler::init},
  {"health_handler", health_handler::init},
  {"markdown_handler", markdown_handler::init},
};

router::router(std::vector<HandlerConfig>& handlers) 
  : handlers_(handlers)
{}

std::unique_ptr<request_handler> router::match(const std::string& path, std::string& log_handler_name) {
  Logger *logger = Logger::get_global_log();

  // Look for a handler with longest matching prefix
  int longest_match = 0;
  std::string handler_name = "";
  std::string root = "";
  for (const auto& handler: handlers_) {
    if (path.find(handler.path) == 0 && handler.path.size() > longest_match) {
      longest_match = handler.path.size();
      handler_name = handler.name;
      root = handler.root;
    }
  }

  logger->logDebug("Received handler " + handler_name);
  request_handler_factory factory = router::handler_registry_[handler_name];
  if (!factory) {
    return nullptr;
  }
  log_handler_name = handler_name;
  return factory(root);
}
