#include <string>
#include <filesystem>
#include <cstdio>
#include <sstream>
#include <boost/beast/http.hpp>
#include <boost/lexical_cast.hpp>
#include "static_handler.h"
#include "markdown_to_html.h"
namespace http = boost::beast::http;

std::unique_ptr<request_handler> static_handler::init(std::string root) {
    return std::make_unique<static_handler>(root);
}

static_handler::static_handler(std::string root): root_(root) {}

http::response<http::string_body> static_handler::handle_request(http::request<http::string_body> request) {
    http::response<http::string_body> response;
    response.version(11);

    std::string filepath = std::string(request.target());

    // Get parameters.
    std::string parameter = "";
    if(filepath.find_last_of("?") != std::string::npos) {
        parameter = filepath.substr(filepath.find_last_of("?") + 1);
        filepath = filepath.substr(0, filepath.find_last_of("?"));
    }

    // File found.
    if(std::filesystem::exists(root_ + filepath) && !std::filesystem::is_directory(root_ + filepath)) {
        response.result(http::status::ok);

        // Determine content type.
        std::string file_extension = "";
        std::string filename = filepath.substr(filepath.find_last_of("/") + 1);
        if(filename.find_last_of(".") != std::string::npos) {
            file_extension = filename.substr(filename.find_last_of(".") + 1);
        }
        if(file_extension == "html" || file_extension == "md") {
            response.set(http::field::content_type, "text/html");
        } else if(file_extension == "jpg" || file_extension == "jpeg") {
            response.set(http::field::content_type, "image/jpeg");
        } else if(file_extension == "txt") {
            response.set(http::field::content_type, "text/plain");
        } else if(file_extension == "zip") {
            response.set(http::field::content_type, "application/zip");
        } else {
            response.set(http::field::content_type, "application/octet-stream");
        }

        // Load file into payload.
        std::FILE* file = std::fopen((root_ + filepath).c_str(), "r");
        std::stringstream payload;
        int current = std::fgetc(file);
        while(current != EOF) {
            payload.put(current);
            current = std::fgetc(file);
        }
        std::fclose(file);

        // Process file if necessary.
        if(file_extension == "md") {
            MarkdownToHtml parser;
            if(parameter == "" || parameter == "raw=false") {
                response.body() = parser.convert(payload.str());
            } else if(parameter == "raw=true") {
                response.set(http::field::content_type, "text/plain");
                response.body() = payload.str();
            } else {
                response.set(http::field::content_type, "text/plain");
                response.result(http::status::bad_request);
                response.body() = "Bad request";
            }
        } else {
            response.body() = payload.str();
        }

    // File not found.
    } else {
        response.result(http::status::not_found);
        response.body() = "File not found";
    }

    response.prepare_payload();
    return response;
}
