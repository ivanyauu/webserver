#include "gtest/gtest.h"
#include <gmock/gmock.h>
#include <string>
#include <echo_handler.h>
#include <static_handler.h>
#include <notfound_handler.h>
#include <crud_handler.h>
#include <sleep_handler.h>
#include <health_handler.h>
#include <markdown_handler.h>
#include <boost/beast/http.hpp>
#include <boost/lexical_cast.hpp>
#include "i_file_io.h"
#include <ios>
#include <memory>
#include <regex>
#include <map>
#include <sstream>
#include <vector>
#include <algorithm>
#include <chrono>

namespace http = boost::beast::http;

class fake_file_io : public i_file_io {
  std::map<std::string, std::stringstream> files;
  std::map<std::string, std::vector<std::string>> keys;
  std::string current_file;
public:
  bool open(const std::string& file_path, std::ios_base::openmode mode) override {
    current_file = file_path;

    // Separate file_path into <parent_dir>/<id>
    std::size_t last_slash_pos = file_path.find_last_of("/");
    std::string parent_dir = file_path.substr(0, last_slash_pos);
    std::string id = file_path.substr(last_slash_pos + 1);

    if ((mode & std::ios::out) && (mode & std::ios::trunc) || (mode & std::ios::out)) {
      if (files.count(file_path)) {
        files[file_path].str(""); // clear file content
      } else {
        // Place key:value <dir>:<file> into keys vector
        keys[parent_dir].push_back(id);

        // Simulate file open with insert file_path into map
        files[file_path];
      }
    } else if (mode & std::ios::in && !files.count(file_path)) {
      return false;
    }
    return true;
  }

  bool write(const std::string& data) override {
    if (!current_file.empty()) {
      files[current_file] << data;
      return true;
    }
    return false; // No file opened
  }

  bool read(const std::string& filepath, std::string& content) override {
    auto check_file = files.find(filepath);
    if (check_file != files.end()) {
      content = check_file->second.str();
      return true;
    }

    return false;
  }

  void close() override { 
    current_file = "";
  }

  bool delete_file(const std::string& filepath) override{
    auto check_file = files.find(filepath);
    if (check_file == files.end()) {
      return false;
    }
    files.erase(check_file);

    // Separate file_path into <parent_dir>/<id>
    std::size_t last_slash_pos = filepath.find_last_of("/");
    std::string parent_dir = filepath.substr(0, last_slash_pos);
    std::string id = filepath.substr(last_slash_pos + 1);

    auto check_keys = std::find(keys[parent_dir].begin(), keys[parent_dir].end(), id);
    if (check_keys == keys[parent_dir].end()) {
      return false;
    }
    keys[parent_dir].erase(check_keys);

    return true;
  }

  bool create_directories(const std::string& path) override {
    // Add path in map if not present. Otherwise false.
    auto it = keys.find(path);
    if (it == keys.end()) {
      keys.emplace(path, std::vector<std::string>());
    } else {
      return false;
    }
    return true;
  }

  bool list_directories(const std::string& path, std::vector<std::string>& directory) override {
    auto it = keys.find(path);
    if(it != keys.end()) {
      directory = it->second;
      return true;
    }
    else {
      return false;
    }
  }

  bool exists(const std::string& filepath) override {
    if (files.count(filepath) || keys.count(filepath)) {
      // If regular file or directory, return true
      return true;
    }
    return false;
  }
};

class EchoHandlerTest : public testing::Test {
protected:
  echo_handler handler;
};

class StaticHandlerTest : public testing::Test {
protected:
  static_handler handler = static_handler("/usr/src/projects/new-grad-ten-years-experience");
};

class NotFoundHandlerTest : public testing::Test {
protected:
  notfound_handler handler;
};

class CrudHandlerTest : public testing::Test {
protected:
  std::shared_ptr<fake_file_io> file_io_ptr = std::make_shared<fake_file_io>();
  crud_handler handler = crud_handler("./root", file_io_ptr);
};

class SleepHandlerTest : public testing::Test {
protected:
  sleep_handler handler;
};

class HealthHandlerTest : public testing::Test {
protected:
  health_handler handler;
};

class MarkdownHandlerTest : public testing::Test {
protected:
  std::shared_ptr<fake_file_io> file_io_ptr = std::make_shared<fake_file_io>();
  markdown_handler handler = markdown_handler("./root", file_io_ptr);
};

TEST_F(NotFoundHandlerTest, NotFoundRequest) {
  std::string validRequest = "GET / HTTP/1.1\r\n\r\n";
  http::request_parser<http::string_body> parser;
  boost::beast::error_code ec;
  parser.put(boost::asio::buffer(validRequest), ec);
  http::request<http::string_body> parsedRequest = parser.release();

  http::response<http::string_body> response = handler.handle_request(parsedRequest);
  ASSERT_EQ(response.result(), http::status::not_found);
  ASSERT_EQ(response.body(), "<html><head><title>Not Found</title></head><body><h1>404 Not Found</h1></body></html>");
}



TEST_F(EchoHandlerTest, ValidHTTPRequest) {
  std::string validRequest = "GET /index.html HTTP/1.1\r\nHost: example.com\r\n\r\n";
  http::request_parser<http::string_body> parser;
  boost::beast::error_code ec;
  parser.put(boost::asio::buffer(validRequest), ec);
  http::request<http::string_body> parsedRequest = parser.release();

  std::string expectedResponse = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 47\r\n\r\nGET /index.html HTTP/1.1\r\nHost: example.com\r\n\r\n";
  ASSERT_EQ(boost::lexical_cast<std::string>(handler.handle_request(parsedRequest)), expectedResponse);
}

TEST_F(EchoHandlerTest, EmptyHTTPRequest) {
  std::string emptyRequest = "";
  http::request_parser<http::string_body> parser;
  boost::beast::error_code ec;
  parser.put(boost::asio::buffer(emptyRequest), ec);
  http::request<http::string_body> parsedRequest = parser.release();

  std::string expectedResponse = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\n HTTP/1.1\r\n\r\n";
  ASSERT_EQ(boost::lexical_cast<std::string>(handler.handle_request(parsedRequest)), expectedResponse);
}

TEST_F(StaticHandlerTest, Exists) {

  std::string request0 = "GET /static/Test0.html HTTP/1.1\r\n";
  http::request_parser<http::string_body> parser0;
  boost::beast::error_code ec0;
  parser0.put(boost::asio::buffer(request0), ec0);
  http::request<http::string_body> parsedRequest0 = parser0.release();

  std::string request1 = "GET /static/Test1.jpg HTTP/1.1\r\n";
  http::request_parser<http::string_body> parser1;
  boost::beast::error_code ec1;
  parser1.put(boost::asio::buffer(request1), ec1);
  http::request<http::string_body> parsedRequest1 = parser1.release();

  std::string request2 = "GET /static/Test2.txt HTTP/1.1\r\n";
  http::request_parser<http::string_body> parser2;
  boost::beast::error_code ec2;
  parser2.put(boost::asio::buffer(request2), ec2);
  http::request<http::string_body> parsedRequest2 = parser2.release();

  std::string request3 = "GET /static/Test3.zip HTTP/1.1\r\n";
  http::request_parser<http::string_body> parser3;
  boost::beast::error_code ec3;
  parser3.put(boost::asio::buffer(request3), ec3);
  http::request<http::string_body> parsedRequest3 = parser3.release();

  std::string request4 = "GET /static/Test4 HTTP/1.1\r\n";
  http::request_parser<http::string_body> parser4;
  boost::beast::error_code ec4;
  parser4.put(boost::asio::buffer(request4), ec4);
  http::request<http::string_body> parsedRequest4 = parser4.release();

  std::string request5 = "GET /static/Test5.md HTTP/1.1\r\n";
  http::request_parser<http::string_body> parser5;
  boost::beast::error_code ec5;
  parser5.put(boost::asio::buffer(request5), ec5);
  http::request<http::string_body> parsedRequest5 = parser5.release();

  std::string responseHTML = boost::lexical_cast<std::string>(handler.handle_request(parsedRequest0));
  std::string responseJPG = boost::lexical_cast<std::string>(handler.handle_request(parsedRequest1));
  std::string responseTXT = boost::lexical_cast<std::string>(handler.handle_request(parsedRequest2));
  std::string responseZip = boost::lexical_cast<std::string>(handler.handle_request(parsedRequest3));
  std::string responseBinary = boost::lexical_cast<std::string>(handler.handle_request(parsedRequest4));

  // Check header.
  ASSERT_EQ(responseHTML.substr(0, responseHTML.find("\r\n\r\n") + 4), "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 119\r\n\r\n");
  ASSERT_EQ(responseJPG.substr(0, responseJPG.find("\r\n\r\n") + 4), "HTTP/1.1 200 OK\r\nContent-Type: image/jpeg\r\nContent-Length: 8064\r\n\r\n");
  ASSERT_EQ(responseTXT.substr(0, responseTXT.find("\r\n\r\n") + 4), "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 6\r\n\r\n");
  ASSERT_EQ(responseZip.substr(0, responseZip.find("\r\n\r\n") + 4), "HTTP/1.1 200 OK\r\nContent-Type: application/zip\r\nContent-Length: 349\r\n\r\n");
  ASSERT_EQ(responseBinary.substr(0, responseBinary.find("\r\n\r\n") + 4), "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: 6\r\n\r\n");

  // Check entire response.
  ASSERT_EQ(responseHTML, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 119\r\n\r\n<!DOCTYPE html>\n<html>\n    <head>\n        <title>TEST</title>\n    </head>\n    <body>\n        STUFF\n    </body>\n</html>\n");
  ASSERT_EQ(responseTXT, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 6\r\n\r\nSTUFF\n");
  ASSERT_EQ(responseBinary, "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: 6\r\n\r\nSTUFF\n");
}

TEST_F(StaticHandlerTest, Markdown) {
  std::string request0 = "GET /static/Test5.md HTTP/1.1\r\n";
  http::request_parser<http::string_body> parser0;
  boost::beast::error_code ec0;
  parser0.put(boost::asio::buffer(request0), ec0);
  http::request<http::string_body> parsedRequest0 = parser0.release();

  std::string request1 = "GET /static/Test5.md?raw=false HTTP/1.1\r\n";
  http::request_parser<http::string_body> parser1;
  boost::beast::error_code ec1;
  parser1.put(boost::asio::buffer(request1), ec1);
  http::request<http::string_body> parsedRequest1 = parser1.release();

  std::string request2 = "GET /static/Test5.md?raw=true HTTP/1.1\r\n";
  http::request_parser<http::string_body> parser2;
  boost::beast::error_code ec2;
  parser2.put(boost::asio::buffer(request2), ec2);
  http::request<http::string_body> parsedRequest2 = parser2.release();

  std::string request3 = "GET /static/Test5.md?badParameter HTTP/1.1\r\n";
  http::request_parser<http::string_body> parser3;
  boost::beast::error_code ec3;
  parser3.put(boost::asio::buffer(request3), ec3);
  http::request<http::string_body> parsedRequest3 = parser3.release();

  std::string responseMarkdown = boost::lexical_cast<std::string>(handler.handle_request(parsedRequest0));
  std::string responseMarkdownRawFalse = boost::lexical_cast<std::string>(handler.handle_request(parsedRequest1));
  std::string responseMarkdownRawTrue = boost::lexical_cast<std::string>(handler.handle_request(parsedRequest2));
  std::string responseMarkdownBadParameter = boost::lexical_cast<std::string>(handler.handle_request(parsedRequest3));

  std::string parsed = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 520\r\n\r\n<h1>TEST</h1>\n<p>HERE IS <em>SOME</em> STUFF AND <strong>MORE</strong> STUFF AND <em><strong>EVEN MORE</strong></em> STUFF</p>\n<h2>LISTS</h2>\n<h3>ORDERED LIST</h3>\n<ol>\n<li>THIS</li>\n<li>IS</li>\n<li>A</li>\n<li>LIST</li>\n</ol>\n<h3>UNORDERED LIST</h3>\n<ul>\n<li>THIS</li>\n<li>IS</li>\n<li>AN</li>\n<li>UNORDERED</li>\n<li>LIST</li>\n</ul>\n<h2>LINK</h2>\n<p>THIS IS A VERY <a href=\"https://www.youtube.com/watch?v=dQw4w9WgXcQ\">IMPORTANT LINK</a></p>\n<h2>CODE</h2>\n<pre><code># THIS IS A BLOCK OF CODE\nsudo rm -rf /\n</code></pre>\n";
  std::string raw = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 363\r\n\r\n# TEST\r\n\r\nHERE IS *SOME* STUFF AND **MORE** STUFF AND ***EVEN MORE*** STUFF\r\n\r\n## LISTS\r\n\r\n### ORDERED LIST\r\n\r\n1. THIS\r\n2. IS\r\n3. A\r\n4. LIST\r\n\r\n### UNORDERED LIST\r\n\r\n- THIS\r\n- IS\r\n- AN\r\n- UNORDERED\r\n- LIST\r\n\r\n## LINK\r\n\r\nTHIS IS A VERY [IMPORTANT LINK](https://www.youtube.com/watch?v=dQw4w9WgXcQ)\r\n\r\n## CODE\r\n\r\n```\r\n# THIS IS A BLOCK OF CODE\r\nsudo rm -rf /\r\n```\r\n";
  std::string badRequest = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 11\r\n\r\nBad request";

  ASSERT_EQ(responseMarkdown, parsed);
  ASSERT_EQ(responseMarkdownRawFalse, parsed);
  ASSERT_EQ(responseMarkdownRawTrue, raw);
  ASSERT_EQ(responseMarkdownBadParameter, badRequest);
}

TEST_F(StaticHandlerTest, DoesNotExist) {
  std::string fileNotFound = "HTTP/1.1 404 Not Found\r\nContent-Length: 14\r\n\r\nFile not found";

  std::string request0 = "GET /nonExistent HTTP/1.1\r\n";
  http::request_parser<http::string_body> parser0;
  boost::beast::error_code ec0;
  parser0.put(boost::asio::buffer(request0), ec0);
  http::request<http::string_body> parsedRequest0 = parser0.release();


  std::string request1 = "GET / HTTP/1.1\r\n";
  http::request_parser<http::string_body> parser1;
  boost::beast::error_code ec1;
  parser1.put(boost::asio::buffer(request1), ec1);
  http::request<http::string_body> parsedRequest1 = parser1.release();


  ASSERT_EQ(boost::lexical_cast<std::string>(handler.handle_request(parsedRequest0)), fileNotFound);
  ASSERT_EQ(boost::lexical_cast<std::string>(handler.handle_request(parsedRequest1)), fileNotFound);
}

TEST_F(CrudHandlerTest, HandleRequestPostSuccess) {
  // Create POST Request
  http::request<http::string_body> req;
  req.method(http::verb::post);
  req.target("/api/Shoes");
  req.version(11);
  req.set(http::field::content_type, "application/json");
  std::string json_body = "{\"brand\": \"Nike\", \"model\": \"Air Max\", \"size\": 10}";
  req.body() = json_body;
  req.prepare_payload();

  http::response<http::string_body> res = handler.handle_request(req);
  std::string res_body = res.body();

  ASSERT_EQ(res.result(), http::status::created);

  auto content_length = res[http::field::content_length].to_string();
  ASSERT_FALSE(content_length.empty());

  size_t content_length_value = std::stoul(content_length);
  ASSERT_EQ(content_length_value, res_body.size());

  std::regex uuid_regex(R"(\{"id": "\b[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[1-5][0-9a-fA-F]{3}-[89abAB][0-9a-fA-F]{3}-[0-9a-fA-F]{12}\b"\})");
  EXPECT_TRUE(std::regex_match(res_body, uuid_regex));
}

TEST_F(CrudHandlerTest, HandleRequestUnsupportedMethod) {
  // Create HEAD Request
  http::request<http::string_body> req;
  req.method(http::verb::head);
  req.target("/api/Shoes");
  req.version(11);
  req.set(http::field::content_type, "application/json");
  std::string json_body = "{\"brand\": \"Nike\", \"model\": \"Air Max\", \"size\": 10}";
  req.body() = json_body;
  req.prepare_payload();

  http::response<http::string_body> res = handler.handle_request(req);
  ASSERT_EQ(res.result(), http::status::method_not_allowed);
  ASSERT_EQ(res.body(), "<html><head><title>Method Not Allowed</title></head><body><h1>405 Method Not Allowed</h1></body></html>");
}

TEST_F(CrudHandlerTest, HandleRequestPostNoBody) {
  // Create POST Request with no body (data)
  http::request<http::string_body> req;
  req.method(http::verb::post);
  req.target("/api/Shoes");
  req.version(11);

  http::response<http::string_body> res = handler.handle_request(req);
  ASSERT_EQ(res.result(), http::status::bad_request);
  ASSERT_EQ(res.body(), "No data in POST request");
}

TEST_F(CrudHandlerTest, HandleRequestPostInvalidContentType) {
  // Create POST Request
  http::request<http::string_body> req;
  req.method(http::verb::post);
  req.target("/api/Shoes");
  req.version(11);
  req.set(http::field::content_type, "text/html");
  std::string json_body = "{\"brand\": \"Nike\", \"model\": \"Pegasus\", \"size\": 7}";
  req.body() = json_body;
  req.prepare_payload();

  http::response<http::string_body> res = handler.handle_request(req);
  ASSERT_EQ(res.result(), http::status::unsupported_media_type);
}

TEST_F(CrudHandlerTest, HandleRequestInvalidURI) {
  // Create POST Request with invalid entity URI
  http::request<http::string_body> req;
  req.method(http::verb::post);
  req.target("/api");
  req.version(11);
  req.set(http::field::content_type, "application/json");
  std::string json_body = "{\"brand\": \"Nike\", \"model\": \"Air Max\", \"size\": 10}";
  req.body() = json_body;
  req.prepare_payload();

  http::response<http::string_body> res = handler.handle_request(req);
  ASSERT_EQ(res.result(), http::status::bad_request);
  ASSERT_EQ(res.body(), "No entity directory specified");
}

TEST_F(CrudHandlerTest, HandleRequestInvalidURIPostRequest) {
  // Create POST Request with invalid entity URI
  http::request<http::string_body> req;
  req.method(http::verb::post);
  req.target("/api/");
  req.version(11);
  req.set(http::field::content_type, "application/json");
  std::string json_body = "{\"brand\": \"Nike\", \"model\": \"Air Max\", \"size\": 10}";
  req.body() = json_body;
  req.prepare_payload();

  http::response<http::string_body> res = handler.handle_request(req);
  ASSERT_EQ(res.result(), http::status::bad_request);
  ASSERT_EQ(res.body(), "No entity directory specified");
}

TEST_F(CrudHandlerTest, HandleRequestGetSuccess) {

  // Test Get 
  
  http::request<http::string_body> req_post;
  req_post.method(http::verb::post);
  req_post.target("/api/Shoes");
  req_post.version(11);
  req_post.set(http::field::content_type, "application/json");
  std::string test_data ="{\"brand\": \"Adidas\", \"model\": \"Gazelle\", \"size\": 9.5}";
  req_post.body() = test_data;
  req_post.prepare_payload();

  http::response<http::string_body> res_post = handler.handle_request(req_post);

  http::request<http::string_body> req_get;
  req_get.method(http::verb::get);

  //Parsing the ID from the POST

  size_t id_pos = res_post.body().find("\"id\": \"");
  size_t apo_start = res_post.body().find_first_of('"', id_pos + 5);
  size_t apo_end = res_post.body().find_first_of('"', apo_start + 1);

  std::string id = res_post.body().substr(apo_start + 1, apo_end - apo_start - 1);

  req_get.target("/api/Shoes/" + id);
  req_get.version(11);
  req_get.set(http::field::content_type, "application/json");
  req_get.prepare_payload();

  http::response<http::string_body> res_get = handler.handle_request(req_get);
  std::string res_body = res_get.body();

  EXPECT_EQ(res_get.result(), http::status::ok);
  EXPECT_EQ(res_body, test_data);
  EXPECT_EQ(res_get[http::field::content_type], "application/json");
  
}

TEST_F(CrudHandlerTest, HandleRequestGetFailure) {

  // Test Get 

  http::request<http::string_body> req_get;
  req_get.method(http::verb::get);

  std::string id = "bad_id";

  req_get.target("/api/Shoes/" + id);
  req_get.version(11);
  req_get.set(http::field::content_type, "application/json");
  req_get.prepare_payload();

  http::response<http::string_body> res_get = handler.handle_request(req_get);
  std::string res_body = res_get.body();

  EXPECT_EQ(res_get.result(), http::status::not_found);
  
}

TEST_F(CrudHandlerTest, HandleRequestGetInvalidURI) {
  // Create GET request
  http::request<http::string_body> req_get;
  req_get.method(http::verb::get);

  req_get.target("/api/");
  req_get.version(11);
  req_get.prepare_payload();

  http::response<http::string_body> res_get = handler.handle_request(req_get);
  std::string res_body = res_get.body();

  EXPECT_EQ(res_get.result(), http::status::bad_request);
}

TEST_F(CrudHandlerTest, HandleRequestGetListSuccess) {

  // Test Get List
  
  http::request<http::string_body> req_post1;
  req_post1.method(http::verb::post);
  req_post1.target("/api/Shoes");
  req_post1.version(11);
  req_post1.set(http::field::content_type, "application/json");
  std::string test_data1 ="{\"brand\": \"Adidas\", \"model\": \"Gazelle\", \"size\": 9.5}";
  req_post1.body() = test_data1;
  req_post1.prepare_payload();

  http::response<http::string_body> res_post1 = handler.handle_request(req_post1);

  http::request<http::string_body> req_post2;
  req_post2.method(http::verb::post);
  req_post2.target("/api/Shoes");
  req_post2.version(11);
  req_post2.set(http::field::content_type, "application/json");
  std::string test_data2 ="{\"brand\": \"Nike\", \"model\": \"Jordan\", \"size\": 7.5}";
  req_post2.body() = test_data2;
  req_post2.prepare_payload();

  http::response<http::string_body> res_post2 = handler.handle_request(req_post2);

  http::request<http::string_body> req_get;
  req_get.method(http::verb::get);
  req_get.target("/api/Shoes");
  req_get.version(11);
  req_get.set(http::field::content_type, "application/json");
  req_get.prepare_payload();

  http::response<http::string_body> res_get = handler.handle_request(req_get);
  std::string res_body = res_get.body();

  size_t pos = res_body.find("[");
  size_t count = 0;
  while(pos != std::string::npos) {
    ++count;
    pos = res_body.find(",", pos + 1);
  }

  EXPECT_EQ(res_get.result(), http::status::ok);
  EXPECT_EQ(count, 2);
  EXPECT_EQ(res_get[http::field::content_type], "application/json");
  
}

TEST_F(CrudHandlerTest, HandleRequestPutCreationSuccess) {
  // Create PUT Request
  http::request<http::string_body> req;
  req.method(http::verb::put);
  req.target("/api/Shoes/2eefac5f-5dfe-44a6-bb6f-8dd994ab1e8d");
  req.version(11);
  req.set(http::field::content_type, "application/json");
  std::string json_body = "{\"brand\": \"Adidas\", \"model\": \"Samba\", \"size\": 9}";
  req.body() = json_body;
  req.prepare_payload();

  http::response<http::string_body> res = handler.handle_request(req);
  std::string res_body = res.body();

  ASSERT_EQ(res.result(), http::status::created);

  auto content_length = res[http::field::content_length].to_string();
  ASSERT_FALSE(content_length.empty());

  size_t content_length_value = std::stoul(content_length);
  ASSERT_EQ(content_length_value, res_body.size());

  std::string expected_id = "2eefac5f-5dfe-44a6-bb6f-8dd994ab1e8d";
  std::string expected_body = (std::ostringstream() << "{\"id\": \"" << expected_id << "\"}").str();
  EXPECT_EQ(res_body, expected_body);
}

TEST_F(CrudHandlerTest, HandleRequestPutUpdateSuccess) {
  // Create a file with data
  file_io_ptr->create_directories(std::string("./root/entities/update"));
  file_io_ptr->open(std::string("./root/entities/update/01aa9510-60a1-4ca1-a3e1-d3321454e8d2"), std::ios::out);
  file_io_ptr->write(std::string("Default text"));
  file_io_ptr->close();

  // Create PUT (Update) Request for the file above
  http::request<http::string_body> req;
  req.method(http::verb::put);
  req.target("/api/entities/update/01aa9510-60a1-4ca1-a3e1-d3321454e8d2");
  req.version(11);
  req.set(http::field::content_type, "application/json");
  std::string json_body = "{\"brand\": \"Adidas\", \"model\": \"Samba\", \"size\": 9}";
  req.body() = json_body;
  req.prepare_payload();

  http::response<http::string_body> res = handler.handle_request(req);
  ASSERT_EQ(res.result(), http::status::no_content);

  // Verify the file content has been updated
  file_io_ptr->open(std::string("./root/entities/update/01aa9510-60a1-4ca1-a3e1-d3321454e8d2"), std::ios::in);

  std::string file_content;
  file_io_ptr->read("./root/entities/update/01aa9510-60a1-4ca1-a3e1-d3321454e8d2", file_content);
  file_io_ptr->close();

  std::string expected_content = json_body;
  ASSERT_EQ(file_content, expected_content);
}

TEST_F(CrudHandlerTest, HandleRequestPutNoBody) {
  // Create PUT Request with no body (data)
  http::request<http::string_body> req;
  req.method(http::verb::put);
  req.target("/api/Shoes");
  req.version(11);

  http::response<http::string_body> res = handler.handle_request(req);
  ASSERT_EQ(res.result(), http::status::bad_request);
  ASSERT_EQ(res.body(), "No data in PUT request");
}

TEST_F(CrudHandlerTest, HandleRequestPutInvalidContentType) {
  // Create PUT Request
  http::request<http::string_body> req;
  req.method(http::verb::put);
  req.target("/api/Shoes");
  req.version(11);
  req.set(http::field::content_type, "text/html");
  std::string json_body = "{\"brand\": \"Nike\", \"model\": \"Pegasus\", \"size\": 7}";
  req.body() = json_body;
  req.prepare_payload();

  http::response<http::string_body> res = handler.handle_request(req);
  ASSERT_EQ(res.result(), http::status::unsupported_media_type);
}

TEST_F(CrudHandlerTest, HandleRequestPutInvalidURI) {
  // Create PUT Request with invalid entity URI
  http::request<http::string_body> req;
  req.method(http::verb::put);
  req.target("/api");
  req.version(11);
  req.set(http::field::content_type, "application/json");
  std::string json_body = "{\"brand\": \"Nike\", \"model\": \"Air Max\", \"size\": 10}";
  req.body() = json_body;
  req.prepare_payload();

  http::response<http::string_body> res = handler.handle_request(req);
  ASSERT_EQ(res.result(), http::status::bad_request);
  ASSERT_EQ(res.body(), "File format must be <crud-prefix>/<entity-dir>/<id>");
}

TEST_F(CrudHandlerTest, HandleRequestPutCreationMissingIDInURI) {
  // Create PUT Request
  http::request<http::string_body> req;
  req.method(http::verb::put);
  req.target("/api/Shoes");
  req.version(11);
  req.set(http::field::content_type, "application/json");
  std::string json_body = "{\"brand\": \"Adidas\", \"model\": \"Handball\", \"size\": 8}";
  req.body() = json_body;
  req.prepare_payload();

  http::response<http::string_body> res = handler.handle_request(req);
  std::string res_body = res.body();

  ASSERT_EQ(res.result(), http::status::bad_request);
  ASSERT_EQ(res.body(), "File format must be <crud-prefix>/<entity-dir>/<id>");
}

TEST_F(CrudHandlerTest, HandleRequestDeleteSuccess) {
  // Create POST request

  http::request<http::string_body> req_post;
  req_post.method(http::verb::post);
  req_post.target("/api/Shoes");
  req_post.version(11);
  req_post.set(http::field::content_type, "application/json");
  std::string test_data ="{\"brand\": \"Adidas\", \"model\": \"Gazelle\", \"size\": 9.5}";
  req_post.body() = test_data;
  req_post.prepare_payload();

  http::response<http::string_body> res_post = handler.handle_request(req_post);

  //Parsing the ID from the POST

  size_t id_pos = res_post.body().find("\"id\": \"");
  size_t apo_start = res_post.body().find_first_of('"', id_pos + 5);
  size_t apo_end = res_post.body().find_first_of('"', apo_start + 1);

  std::string id = res_post.body().substr(apo_start + 1, apo_end - apo_start - 1);

  // Check that item was created successfully

  http::request<http::string_body> req_get;
  req_get.method(http::verb::get);
  req_get.target("/api/Shoes/" + id);
  req_get.version(11);
  req_get.set(http::field::content_type, "application/json");
  req_get.prepare_payload();

  http::response<http::string_body> res_get = handler.handle_request(req_get);
  std::string res_body = res_get.body();

  ASSERT_EQ(res_get.result(), http::status::ok);
  ASSERT_EQ(res_body, test_data);
  ASSERT_EQ(res_get[http::field::content_type], "application/json");

  // Create DELETE request

  http::request<http::string_body> req_del;
  req_del.method(http::verb::delete_);
  req_del.target("/api/Shoes/" + id);
  req_del.version(11);
  req_del.set(http::field::content_type, "application/json");
  req_del.prepare_payload();

  http::response<http::string_body> res_del = handler.handle_request(req_del);
  ASSERT_EQ(res_del.result(), http::status::no_content);

  // Check that item no longer exists

  http::response<http::string_body> res_get_2 = handler.handle_request(req_get);
  EXPECT_EQ(res_get_2.result(), http::status::not_found);
}

TEST_F(CrudHandlerTest, HandleRequestDeleteFailure) {
  // Create DELETE request

  std::string id = "bad_id";

  http::request<http::string_body> req_del;
  req_del.method(http::verb::delete_);
  req_del.target("/api/Shoes/" + id);
  req_del.version(11);
  req_del.set(http::field::content_type, "application/json");
  req_del.prepare_payload();

  // Check that response is No Content

  http::response<http::string_body> res_del = handler.handle_request(req_del);
  ASSERT_EQ(res_del.result(), http::status::no_content);
}

TEST_F(CrudHandlerTest, HandleRequestDeleteCreationMissingIDInURI) {
  // Create DELETE Request
  http::request<http::string_body> req;
  req.method(http::verb::delete_);
  req.target("/api/Shoes");
  req.version(11);
  req.set(http::field::content_type, "application/json");
  std::string json_body = "{\"brand\": \"Nike\", \"model\": \"Air Max\", \"size\": 3}";
  req.body() = json_body;
  req.prepare_payload();

  http::response<http::string_body> res = handler.handle_request(req);

  ASSERT_EQ(res.result(), http::status::bad_request);
}

TEST_F(SleepHandlerTest, SleepTest) {
  std::string validRequest = "GET /sleep HTTP/1.1\r\n\r\n";
  http::request_parser<http::string_body> parser;
  boost::beast::error_code ec;
  parser.put(boost::asio::buffer(validRequest), ec);
  http::request<http::string_body> parsedRequest = parser.release();

  auto begin = std::chrono::high_resolution_clock::now();
  http::response<http::string_body> result = (handler.handle_request(parsedRequest));
  auto end = std::chrono::high_resolution_clock::now();

  ASSERT_EQ(result.result(), http::status::ok);
  ASSERT_GE(end - begin, std::chrono::seconds(1));
}

TEST_F(HealthHandlerTest, HealthTest) {
  std::string validRequest = "GET /health HTTP/1.1\r\n\r\n";
  http::request_parser<http::string_body> parser;
  boost::beast::error_code ec;
  parser.put(boost::asio::buffer(validRequest), ec);
  http::request<http::string_body> parsedRequest = parser.release();

  http::response<http::string_body> response = handler.handle_request(parsedRequest);

  ASSERT_EQ(response.result(), http::status::ok);
  ASSERT_EQ(response.body(), "OK");
}

TEST_F(MarkdownHandlerTest, CreateMarkdownFileSuccess) {
  http::request<http::string_body> req;
  req.method(http::verb::post);
  req.target("/markdown/Shoes");
  req.version(11);
  req.set(http::field::content_type, "text/markdown");
  std::string text_body = "foo bar";
  req.body() = text_body;
  req.prepare_payload();

  http::response<http::string_body> res = handler.handle_request(req);
  std::string res_body = res.body();

  ASSERT_EQ(res.result(), http::status::created);

  auto content_length = res[http::field::content_length].to_string();
  ASSERT_FALSE(content_length.empty());

  size_t content_length_value = std::stoul(content_length);
  ASSERT_EQ(content_length_value, res_body.size());

  std::regex uuid_regex(R"(\{"id": "\b[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[1-5][0-9a-fA-F]{3}-[89abAB][0-9a-fA-F]{3}-[0-9a-fA-F]{12}\b"\})");
  EXPECT_TRUE(std::regex_match(res_body, uuid_regex));
}

TEST_F(MarkdownHandlerTest, HandleRequestUnsupportedMethod) {
  http::request<http::string_body> req;
  req.method(http::verb::head);
  req.target("/markdown/Shoes");
  req.version(11);
  req.set(http::field::content_type, "text/markdown");
  std::string text_body = "foo bar";
  req.body() = text_body;
  req.prepare_payload();

  http::response<http::string_body> res = handler.handle_request(req);
  ASSERT_EQ(res.result(), http::status::method_not_allowed);
  ASSERT_EQ(res.body(), "<html><head><title>Method Not Allowed</title></head><body><h1>405 Method Not Allowed</h1></body></html>");
}

TEST_F(MarkdownHandlerTest, CreateMarkdownFileInvalidContentType) {
  http::request<http::string_body> req;
  req.method(http::verb::post);
  req.target("/markdown/Shoes");
  req.version(11);
  req.set(http::field::content_type, "text/html");
  std::string text_body = "foo bar";
  req.body() = text_body;
  req.prepare_payload();

  http::response<http::string_body> res = handler.handle_request(req);
  ASSERT_EQ(res.result(), http::status::unsupported_media_type);
}

TEST_F(MarkdownHandlerTest, HandleRequestInvalidURI1) {
  http::request<http::string_body> req;
  req.method(http::verb::post);
  req.target("/markdown");
  req.version(11);
  req.set(http::field::content_type, "text/markdown");
  std::string text_body = "foo bar";
  req.body() = text_body;
  req.prepare_payload();

  http::response<http::string_body> res = handler.handle_request(req);
  ASSERT_EQ(res.result(), http::status::bad_request);
  ASSERT_EQ(res.body(), "No entity directory specified");
}

TEST_F(MarkdownHandlerTest, HandleRequestInvalidURI2) {
  // Create POST Request with invalid entity URI
  http::request<http::string_body> req;
  req.method(http::verb::post);
  req.target("/markdown/");
  req.version(11);
  req.set(http::field::content_type, "text/markdown");
  std::string text_body = "foo bar";
  req.body() = text_body;
  req.prepare_payload();

  http::response<http::string_body> res = handler.handle_request(req);
  ASSERT_EQ(res.result(), http::status::bad_request);
  ASSERT_EQ(res.body(), "No entity directory specified");
}

TEST_F(MarkdownHandlerTest, GetMarkdownFileSuccess) {  
  http::request<http::string_body> req_post;
  req_post.method(http::verb::post);
  req_post.target("/markdown/Shoes");
  req_post.version(11);
  req_post.set(http::field::content_type, "text/markdown");
  std::string test_data ="foo bar";
  std::string expected_response = "<p>foo bar</p>\n";
  req_post.body() = test_data;
  req_post.prepare_payload();

  http::response<http::string_body> res_post = handler.handle_request(req_post);

  http::request<http::string_body> req_get;
  req_get.method(http::verb::get);

  //Parsing the ID from the POST
  size_t id_pos = res_post.body().find("\"id\": \"");
  size_t apo_start = res_post.body().find_first_of('"', id_pos + 5);
  size_t apo_end = res_post.body().find_first_of('"', apo_start + 1);

  std::string id = res_post.body().substr(apo_start + 1, apo_end - apo_start - 1);

  req_get.target("/markdown/Shoes/" + id);
  req_get.version(11);
  req_get.prepare_payload();

  http::response<http::string_body> res_get = handler.handle_request(req_get);
  std::string res_body = res_get.body();

  EXPECT_EQ(res_get.result(), http::status::ok);
  EXPECT_EQ(res_body, expected_response);
  EXPECT_EQ(res_get[http::field::content_type], "text/html"); 
}

TEST_F(MarkdownHandlerTest, GetMarkdownFileRawSuccess) {  
  http::request<http::string_body> req_post;
  req_post.method(http::verb::post);
  req_post.target("/markdown/Shoes");
  req_post.version(11);
  req_post.set(http::field::content_type, "text/markdown");
  std::string test_data ="foo bar";
  req_post.body() = test_data;
  req_post.prepare_payload();

  http::response<http::string_body> res_post = handler.handle_request(req_post);

  http::request<http::string_body> req_get;
  req_get.method(http::verb::get);

  //Parsing the ID from the POST
  size_t id_pos = res_post.body().find("\"id\": \"");
  size_t apo_start = res_post.body().find_first_of('"', id_pos + 5);
  size_t apo_end = res_post.body().find_first_of('"', apo_start + 1);

  std::string id = res_post.body().substr(apo_start + 1, apo_end - apo_start - 1);

  req_get.target("/markdown/Shoes/" + id + "?raw=true");
  req_get.version(11);
  req_get.prepare_payload();

  http::response<http::string_body> res_get = handler.handle_request(req_get);
  std::string res_body = res_get.body();

  EXPECT_EQ(res_get.result(), http::status::ok);
  EXPECT_EQ(res_body, test_data);
  EXPECT_EQ(res_get[http::field::content_type], "text/markdown");
  
}

TEST_F(MarkdownHandlerTest, GetMarkdownFileNotFound) {
  http::request<http::string_body> req_get;
  req_get.method(http::verb::get);

  std::string id = "bad_id";

  req_get.target("/markdown/Shoes/" + id);
  req_get.version(11);
  req_get.prepare_payload();

  http::response<http::string_body> res_get = handler.handle_request(req_get);
  std::string res_body = res_get.body();

  EXPECT_EQ(res_get.result(), http::status::not_found);
}

TEST_F(MarkdownHandlerTest, GetMarkdownFileInvalidURI) {
  // Create GET request
  http::request<http::string_body> req_get;
  req_get.method(http::verb::get);

  req_get.target("/markdown/");
  req_get.version(11);
  req_get.prepare_payload();

  http::response<http::string_body> res_get = handler.handle_request(req_get);
  std::string res_body = res_get.body();

  EXPECT_EQ(res_get.result(), http::status::bad_request);
}

TEST_F(MarkdownHandlerTest, HandleRequestGetListSuccess) {
  // Test Get List
  http::request<http::string_body> req_post1;
  req_post1.method(http::verb::post);
  req_post1.target("/markdown/Shoes");
  req_post1.version(11);
  req_post1.set(http::field::content_type, "text/markdown");
  std::string test_data1 ="foo bar";
  req_post1.body() = test_data1;
  req_post1.prepare_payload();

  http::response<http::string_body> res_post1 = handler.handle_request(req_post1);

  http::request<http::string_body> req_post2;
  req_post2.method(http::verb::post);
  req_post2.target("/markdown/Shoes");
  req_post2.version(11);
  req_post2.set(http::field::content_type, "text/markdown");
  std::string test_data2 ="bar baz";
  req_post2.body() = test_data2;
  req_post2.prepare_payload();

  http::response<http::string_body> res_post2 = handler.handle_request(req_post2);

  http::request<http::string_body> req_get;
  req_get.method(http::verb::get);
  req_get.target("/markdown/Shoes");
  req_get.version(11);
  req_get.prepare_payload();

  http::response<http::string_body> res_get = handler.handle_request(req_get);
  std::string res_body = res_get.body();

  size_t pos = res_body.find("[");
  size_t count = 0;
  while(pos != std::string::npos) {
    ++count;
    pos = res_body.find(",", pos + 1);
  }

  EXPECT_EQ(res_get.result(), http::status::ok);
  EXPECT_EQ(count, 2);
  EXPECT_EQ(res_get[http::field::content_type], "application/json");
  
}

TEST_F(MarkdownHandlerTest, UpdateMarkdownFileCreationSuccess) {
  // Create PUT Request
  http::request<http::string_body> req;
  req.method(http::verb::put);
  req.target("/markdown/Shoes/2eefac5f-5dfe-44a6-bb6f-8dd994ab1e8d");
  req.version(11);
  req.set(http::field::content_type, "text/markdown");
  std::string text_body = "foo bar";
  req.body() = text_body;
  req.prepare_payload();

  http::response<http::string_body> res = handler.handle_request(req);
  std::string res_body = res.body();

  ASSERT_EQ(res.result(), http::status::created);

  auto content_length = res[http::field::content_length].to_string();
  ASSERT_FALSE(content_length.empty());

  size_t content_length_value = std::stoul(content_length);
  ASSERT_EQ(content_length_value, res_body.size());

  std::string expected_id = "2eefac5f-5dfe-44a6-bb6f-8dd994ab1e8d";
  std::string expected_body = (std::ostringstream() << "{\"id\": \"" << expected_id << "\"}").str();
  EXPECT_EQ(res_body, expected_body);
}

TEST_F(MarkdownHandlerTest, UpdateMarkdownFileSuccess) {
  // Create a file with data
  file_io_ptr->create_directories(std::string("./root/entities/update"));
  file_io_ptr->open(std::string("./root/entities/update/01aa9510-60a1-4ca1-a3e1-d3321454e8d2"), std::ios::out);
  file_io_ptr->write(std::string("Default text"));
  file_io_ptr->close();

  // Create PUT (Update) Request for the file above
  http::request<http::string_body> req;
  req.method(http::verb::put);
  req.target("/markdown/entities/update/01aa9510-60a1-4ca1-a3e1-d3321454e8d2");
  req.version(11);
  req.set(http::field::content_type, "text/markdown");
  std::string text_body = "foo bar";
  req.body() = text_body;
  req.prepare_payload();

  http::response<http::string_body> res = handler.handle_request(req);
  ASSERT_EQ(res.result(), http::status::no_content);

  // Verify the file content has been updated
  file_io_ptr->open(std::string("./root/entities/update/01aa9510-60a1-4ca1-a3e1-d3321454e8d2"), std::ios::in);

  std::string file_content;
  file_io_ptr->read("./root/entities/update/01aa9510-60a1-4ca1-a3e1-d3321454e8d2", file_content);
  file_io_ptr->close();

  std::string expected_content = text_body;
  ASSERT_EQ(file_content, expected_content);
}

TEST_F(MarkdownHandlerTest, UpdateMarkdownFileInvalidContentType) {
  // Create PUT Request
  http::request<http::string_body> req;
  req.method(http::verb::put);
  req.target("/markdown/Shoes");
  req.version(11);
  req.set(http::field::content_type, "text/html");
  std::string text_body = "foo bar";
  req.body() = text_body;
  req.prepare_payload();

  http::response<http::string_body> res = handler.handle_request(req);
  ASSERT_EQ(res.result(), http::status::unsupported_media_type);
}

TEST_F(MarkdownHandlerTest, UpdateMarkdownFileInvalidURI) {
  // Create PUT Request with invalid entity URI
  http::request<http::string_body> req;
  req.method(http::verb::put);
  req.target("/markdown");
  req.version(11);
  req.set(http::field::content_type, "text/markdown");
  std::string text_body = "foo bar";
  req.body() = text_body;
  req.prepare_payload();

  http::response<http::string_body> res = handler.handle_request(req);
  ASSERT_EQ(res.result(), http::status::bad_request);
  ASSERT_EQ(res.body(), "File format must be <markdown-prefix>/<entity-dir>/<id>");
}

TEST_F(MarkdownHandlerTest, UpdateMarkdownRequestMissingIDInURI) {
  // Create PUT Request
  http::request<http::string_body> req;
  req.method(http::verb::put);
  req.target("/markdown/Shoes");
  req.version(11);
  req.set(http::field::content_type, "text/markdown");
  std::string text_body = "foo bar";
  req.body() = text_body;
  req.prepare_payload();

  http::response<http::string_body> res = handler.handle_request(req);
  std::string res_body = res.body();

  ASSERT_EQ(res.result(), http::status::bad_request);
  ASSERT_EQ(res.body(), "File format must be <markdown-prefix>/<entity-dir>/<id>");
}

TEST_F(MarkdownHandlerTest, DeleteMarkdownFileSuccess) {
  http::request<http::string_body> req_post;
  req_post.method(http::verb::post);
  req_post.target("/markdown/Shoes");
  req_post.version(11);
  req_post.set(http::field::content_type, "text/markdown");
  std::string test_data ="foo bar";
  req_post.body() = test_data;
  req_post.prepare_payload();

  http::response<http::string_body> res_post = handler.handle_request(req_post);

  //Parsing the ID from the POST
  size_t id_pos = res_post.body().find("\"id\": \"");
  size_t apo_start = res_post.body().find_first_of('"', id_pos + 5);
  size_t apo_end = res_post.body().find_first_of('"', apo_start + 1);

  std::string id = res_post.body().substr(apo_start + 1, apo_end - apo_start - 1);

  // Check that item was created successfully
  http::request<http::string_body> req_get;
  req_get.method(http::verb::get);
  req_get.target("/markdown/Shoes/" + id + "?raw=true");
  req_get.version(11);
  req_get.prepare_payload();

  http::response<http::string_body> res_get = handler.handle_request(req_get);
  std::string res_body = res_get.body();

  ASSERT_EQ(res_get.result(), http::status::ok);
  ASSERT_EQ(res_body, test_data);
  ASSERT_EQ(res_get[http::field::content_type], "text/markdown");

  // Create DELETE request
  http::request<http::string_body> req_del;
  req_del.method(http::verb::delete_);
  req_del.target("/markdown/Shoes/" + id);
  req_del.version(11);
  req_del.prepare_payload();

  http::response<http::string_body> res_del = handler.handle_request(req_del);
  ASSERT_EQ(res_del.result(), http::status::no_content);

  // Check that item no longer exists
  http::response<http::string_body> res_get_2 = handler.handle_request(req_get);
  EXPECT_EQ(res_get_2.result(), http::status::not_found);
}

TEST_F(MarkdownHandlerTest, DeleteMarkdownFileFailure) {
  // Create DELETE request
  std::string id = "bad_id";

  http::request<http::string_body> req_del;
  req_del.method(http::verb::delete_);
  req_del.target("/markdown/Shoes/" + id);
  req_del.version(11);
  req_del.prepare_payload();

  // Check that response is No Content
  http::response<http::string_body> res_del = handler.handle_request(req_del);
  ASSERT_EQ(res_del.result(), http::status::no_content);
}

TEST_F(MarkdownHandlerTest, DeleteMarkdownFileMissingIDInURI) {
  // Create DELETE Request
  http::request<http::string_body> req;
  req.method(http::verb::delete_);
  req.target("/markdown/Shoes");
  req.version(11);
  req.prepare_payload();

  http::response<http::string_body> res = handler.handle_request(req);

  ASSERT_EQ(res.result(), http::status::bad_request);
}


