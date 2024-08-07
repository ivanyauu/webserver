#ifndef MARKDOWN_HANDLER_H
#define MARKDOWN_HANDLER_H

#include "request_handler.h"
#include "i_file_io.h"
#include <string>
#include <vector>
#include <memory>
#include <filesystem>

class markdown_handler: public request_handler {
public:
    static std::unique_ptr<request_handler> init(std::string data_path);
    markdown_handler(std::string data_path, std::shared_ptr<i_file_io> file_io_ptr);
    http::response<http::string_body> handle_request(http::request<http::string_body> request) override;

private:
    std::string data_path_;
    std::shared_ptr<i_file_io> file_io_;
    std::string generate_id();
    // handle_request delegates to specific HTTP method
    http::response<http::string_body> create_markdown_file(const http::request<http::string_body>& request);
    http::response<http::string_body> get_markdown_file(const http::request<http::string_body>& request);
    http::response<http::string_body> update_markdown_file(const http::request<http::string_body>& request);
    http::response<http::string_body> delete_markdown_file(const http::request<http::string_body>& request);
    
    // helper functions
    bool create_or_update_entity(const std::filesystem::path& path, const std::string& entity_data);
    std::string remove_prefix_dir(boost::beast::string_view prefix, boost::beast::string_view uri_view);
    int count_path_segments(const std::filesystem::path& path);
    bool has_markdown_content_type(const http::request<http::string_body>& req);
    http::response<http::string_body> create_response(http::status status, const std::string& content_type, const std::string& body);
};

#endif // MARKDOWN_HANDLER_H
