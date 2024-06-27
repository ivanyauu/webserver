#include "gtest/gtest.h"
#include <gmock/gmock.h>
#include <iostream>
#include <memory>
#include <router.h>
#include <echo_handler.h>
#include <static_handler.h>
#include <crud_handler.h>
#include <string>
#include <vector>

class RouterTest : public ::testing::Test {
protected:
  router* router_instance;

  void SetUp() override {
    std::vector<HandlerConfig> handlers = {
      {
        "echo_handler", // name
        "/echo", // path
        "", // root
      },
      {
        "static_handler", // name
        "/echo/static", // path
        "", // root
      },
      {
        "crud_handler", // name
        "/api", // path
        "", // root
      }
    };
    router_instance = new router(handlers);
  }

};

TEST_F(RouterTest, UnhandledPath) {
  std::string path = "/unknown";
  std::string handler_name = "";
  ASSERT_EQ(router_instance->match(path, handler_name), nullptr);
}

TEST_F(RouterTest, LongestMatch) {
  std::string path = "/echo/static";
  std::string handler_name = "";
  std::shared_ptr<request_handler> handler = router_instance->match(path, handler_name);
  
  ASSERT_NE(std::dynamic_pointer_cast<static_handler>(handler), nullptr);
}

TEST_F(RouterTest, ExactMatch) {
  std::string path = "/echo";
  std::string handler_name = "";
  std::shared_ptr<request_handler> handler = router_instance->match(path, handler_name);
  
  ASSERT_NE(std::dynamic_pointer_cast<echo_handler>(handler), nullptr);
}

TEST_F(RouterTest, CrudHandlerMatch) {
  std::string path = "/api/Shoes";
  std::string handler_name = "";
  std::shared_ptr<request_handler> handler = router_instance->match(path, handler_name);

  ASSERT_NE(std::dynamic_pointer_cast<crud_handler>(handler), nullptr);
}
