#include "gtest/gtest.h"
#include <gmock/gmock.h>
#include <boost/asio.hpp>
#include <session.h>
#include <vector>
#include <string>

class SessionTest : public ::testing::Test {
protected:
  boost::asio::io_service io_service;
  session* session_instance;

  void SetUp() override {
    std::vector<HandlerConfig> handlers = {
      {
        "echo_handler", // name
        "/echo", // path
        "", // root
      }
    };
    session_instance = new session(io_service, handlers);
  }

};

TEST_F(SessionTest, ReadRequestWithErrorTest) {
  // Note that session doesn't throw an error with an error code, just deletes the obj and returns gracefully //
  EXPECT_NO_THROW(session_instance->handle_read(boost::system::errc::make_error_code(boost::system::errc::io_error), 0));
}

TEST_F(SessionTest, WriteRequestWithErrorTest) {
  EXPECT_NO_THROW(session_instance->handle_write(boost::system::errc::make_error_code(boost::system::errc::io_error)));
}

TEST_F(SessionTest, HandleReadTest) {
  std::string request = "GET / HTTP/1.1\r\n";
  std::string expectedResponse = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n" + request;

  EXPECT_NO_THROW(session_instance->handle_read(boost::system::error_code(), request.size()));
}

TEST_F(SessionTest, WriteRequestTest) {
  std::vector<HandlerConfig> handlers = {
    {
      "echo_handler", // name
      "/echo", // path
      "", // root
    }
  };
  session testSession(io_service, handlers);

  EXPECT_NO_THROW(session_instance->handle_write(boost::system::error_code()));
}

TEST_F(SessionTest, ConstructorTest) {
  std::vector<HandlerConfig> handlers = {
    {
      "echo_handler", // name
      "/echo", // path
      "", // root
    }
  };
  EXPECT_NO_THROW(session session_instance(io_service, handlers));
}

TEST_F(SessionTest, SocketTest) {
  EXPECT_NO_THROW(session_instance->socket());
}

TEST_F(SessionTest, StartTest) {
  EXPECT_NO_THROW(session_instance->start());
}

TEST_F(SessionTest, UnhandledPath) {
  http::request<http::string_body> req;
  req.method(http::verb::get);
  req.target("/unknown");
  req.version(11);  
  std::string expectedResponse = "HTTP/1.1 500 Internal Server Error\r\n\r\n";

  ASSERT_EQ(session_instance->process_request(req), expectedResponse);
}
