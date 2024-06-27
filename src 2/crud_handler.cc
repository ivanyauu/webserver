#include <exception>
#include <ios>
#include <stdexcept>
#include <string>
#include <sstream>
#include <filesystem>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <memory>
#include <boost/beast/http.hpp>
#include <boost/lexical_cast.hpp>
#include "logger.h"
#include "file_io.h"
#include "crud_handler.h"
namespace http = boost::beast::http;

std::unique_ptr<request_handler> crud_handler::init(std::string data_path) {
    auto file_io_ptr = std::make_shared<file_io>();
    return std::make_unique<crud_handler>(data_path, file_io_ptr);
}

crud_handler::crud_handler(std::string data_path, std::shared_ptr<i_file_io> file_io_ptr): data_path_(data_path), file_io_(file_io_ptr) {}

http::response<http::string_body> crud_handler::handle_request(http::request<http::string_body> request) {
    switch (request.method()) {
        case http::verb::post:
            return handle_post_request(request);
        case http::verb::get:
            return handle_get_request(request);
        case http::verb::put:
            return handle_put_request(request);
        case http::verb::delete_:
            return handle_delete_request(request);
        default:
            return create_response(http::status::method_not_allowed,
                                    "text/html",
                                    "<html><head><title>Method Not Allowed</title></head><body><h1>405 Method Not Allowed</h1></body></html>");
    }
}

std::string crud_handler::generate_id() {
    static boost::uuids::random_generator generator;
    boost::uuids::uuid id = generator();
    return boost::uuids::to_string(id);
}

http::response<http::string_body> crud_handler::handle_post_request(const http::request<http::string_body>& request) {
    Logger *logger = Logger::get_global_log();
    if (request.body().size() == 0) {
        logger->logError("ERROR: No data in POST request");
        return create_response(http::status::bad_request,
                                "text/plain",
                                "No data in POST request");
    }

    if (!has_json_content_type(request)) {
        logger->logError("ERROR: Content-Type must be application/json");
        return create_response(http::status::unsupported_media_type,
                                "text/plain",
                                "Content-Type must be application/json");
    }

    std::string entity_dir = remove_prefix_dir("/api/", request.target());
    if (entity_dir.empty()) {
        logger->logError("ERROR: No entity directory specified");
        return create_response(http::status::bad_request,
                                "text/plain",
                                "No entity directory specified");
    }

    std::string id = generate_id();
    std::filesystem::path entity_path = std::filesystem::path(data_path_) / std::filesystem::path(entity_dir) / std::filesystem::path(id);

    logger->logDebug("Root: " + data_path_);
    logger->logDebug("Uri: " + entity_dir);
    logger->logDebug("Id: " + id);
    logger->logDebug("Filepath: " + entity_path.string());

    std::string entity_data = request.body();
    bool success = create_or_update_entity(entity_path, entity_data);
    if (!success) {
        return create_response(http::status::internal_server_error,
                                "text/plain",
                                "Unable to create file. Please try again later.");
    }
    logger->logDebug("JSON data written successfully: " + entity_data);

    std::string body = (std::ostringstream() << "{\"id\": \"" << id << "\"}").str();
    return create_response(http::status::created,
                            "application/json",
                            body);
}

http::response<http::string_body> crud_handler::handle_get_request(const http::request<http::string_body>& request) {
    Logger *logger = Logger::get_global_log();

    std::string entity_with_id = remove_prefix_dir("/api/", request.target());
    if (entity_with_id.empty()) {
        logger->logError("ERROR: No entity directory specified");
        return create_response(http::status::bad_request,
                                "text/html",
                                "<html><head><title>Bad Request</title></head><body><h1>400 Bad Request</h1></body></html>");
    }

    size_t pos_slash = entity_with_id.find_last_of('/');
    std::string entity;
    std::string id;
    
    if (pos_slash != std::string::npos) {
        entity = entity_with_id.substr(0, pos_slash);
        id = entity_with_id.substr(pos_slash+1);
    }
    else {
        entity = entity_with_id;
    }

    std::filesystem::path entity_path = std::filesystem::path(data_path_) / std::filesystem::path(entity_with_id);

    logger->logDebug("Root: " + data_path_);
    logger->logDebug("Entity: " + entity);
    logger->logDebug("ID: " + id);
    logger->logDebug("Filepath: " + entity_path.string());

    if (id.empty()) {
        // No ID found, try listing all the filenames for the given entity
        logger->logDebug("Listing filenames in the entity path of " + (entity_path).string());

        std::vector<std::string> ids;
        if(!file_io_->list_directories(std::string(entity_path), ids)) {
            return create_response(http::status::bad_request,
                                    "text/html",
                                    "<html><head><title>Bad Request</title></head><body><h1>400 Bad Request</h1></body></html>");
        }

        std::ostringstream oss;
        oss << "[";
        for(size_t i = 0; i< ids.size(); ++i) {
            if (i>=1) oss << ",";
            oss << "\"" << ids[i] << "\"";
        }
        oss << "]";

        std::string body = oss.str();
        return create_response(http::status::ok,
                                "application/json",
                                body);
    }
    
    std::string entity_data;

    // Failed to open
    if (!file_io_->open(std::string(entity_path), std::ios::in)) {
        logger->logError("ERROR: Failed to open file at " + entity_path.string());
        return create_response(http::status::not_found,
                                "text/html",
                                "<html><head><title>Not Found</title></head><body><h1>404 Not Found</h1></body></html>");
    }

    // Failed to Read
    if (!file_io_->read(std::string(entity_path), entity_data)) {
        logger->logError("ERROR: Failed to read file");
        return create_response(http::status::internal_server_error,
                                "text/html",
                                "<html><head><title>Internal Server Error</title></head><body><h1>500 Internal Server Error</h1></body></html>");
    }

    file_io_->close();
    logger->logDebug("JSON data retrieved successfully: " + entity_data);

    std::string body = entity_data;
    return create_response(http::status::ok,
                            "application/json",
                            body);
}

http::response<http::string_body> crud_handler::handle_put_request(const http::request<http::string_body>& request) {
    Logger *logger = Logger::get_global_log();
    if (request.body().size() == 0) {
        logger->logError("ERROR: No data in PUT request");
        return create_response(http::status::bad_request,
                                "text/plain",
                                "No data in PUT request");
    }
    
    if (!has_json_content_type(request)) {
        logger->logError("ERROR: Content-Type must be application/json");
        return create_response(http::status::unsupported_media_type,
                                "text/plain",
                                "Content-Type must be application/json");
    }

    std::string entity_dir = remove_prefix_dir("/api/", request.target());
    int path_segments = count_path_segments(std::filesystem::path(entity_dir));
    if (entity_dir.empty() || path_segments < 2) {
        logger->logError("ERROR: File has invalid path: " + entity_dir);
        return create_response(http::status::bad_request,
                                "text/plain",
                                "File format must be <crud-prefix>/<entity-dir>/<id>");
    }
    
    std::filesystem::path entity_path = std::filesystem::path(data_path_) / entity_dir;
    bool is_new_file = !file_io_->exists(std::string(entity_path));

    std::string entity_data = request.body();
    bool success = create_or_update_entity(entity_path, entity_data);
    if (!success) {
        return create_response(http::status::internal_server_error,
                                "text/plain",
                                "Unable to create or update file. Please try again later.");
    }

    std::string id = entity_path.filename().string();
    logger->logDebug("Request file id: " + id);

    if (is_new_file) {
        // file creation response
        std::string body = (std::ostringstream() << "{\"id\": \"" << id << "\"}").str();
        return create_response(http::status::created,
                                "application/json",
                                body);
    } else {
        // update existing file response
        std::string body = "";
        return create_response(http::status::no_content,
                                "application/json",
                                body);
    }
}

http::response<http::string_body> crud_handler::handle_delete_request(const http::request<http::string_body>& request) {
    Logger *logger = Logger::get_global_log();

    std::string entity_with_id = remove_prefix_dir("/api/", request.target());
    int path_segments = count_path_segments(std::filesystem::path(entity_with_id));
    if (entity_with_id.empty() || path_segments < 2) {
        logger->logError("ERROR: File has invalid path: " + entity_with_id);
        return create_response(http::status::bad_request,
                                "text/plain",
                                "File format must be <crud-prefix>/<entity-dir>/<id>");
    }

    size_t pos_slash = entity_with_id.find_last_of('/');
    std::string entity;
    std::string id;
    
    if (pos_slash != std::string::npos) {
        entity = entity_with_id.substr(0, pos_slash);
        id = entity_with_id.substr(pos_slash+1);
    }
    else {
        entity = entity_with_id;
    }

    std::filesystem::path entity_path = std::filesystem::path(data_path_) / std::filesystem::path(entity_with_id);

    logger->logDebug("Root: " + data_path_);
    logger->logDebug("Entity: " + entity);
    logger->logDebug("ID: " + id);
    logger->logDebug("Filepath: " + entity_path.string());

    if (!file_io_->delete_file(std::string(entity_path))) {
        logger->logError("ERROR: Cannot delete file that does not exist at " + entity_path.string());
    }
    else {
        logger->logDebug("Successfully deleted file");
    }

    file_io_->close();

    std::string body = "";
    return create_response(http::status::no_content,
                            "application/json",
                            body);
}

bool crud_handler::create_or_update_entity(const std::filesystem::path& path, const std::string& entity_data) {
    Logger *logger = Logger::get_global_log();
    try {
        if (file_io_->create_directories(std::string(path.parent_path()))) {
            logger->logDebug("Created base directories: " + (path.parent_path()).string());
        }
    } catch (const std::exception& e) {
        logger->logError("ERROR: Failed to create directories: " + (path.parent_path()).string());
        return false;
    }

    // Create file if it does not exist, or clear file contents
    if (!file_io_->open(std::string(path), std::ios::out | std::ios::trunc)) {
        logger->logError("ERROR: Failed to open file at " + path.string());
        return false;
    }

    if (!file_io_->write(entity_data)) {
        logger->logError("ERROR: Failed to write to file at " + path.string());
        return false;
    }

    file_io_->close();
    return true;
}

std::string crud_handler::remove_prefix_dir(boost::beast::string_view prefix, boost::beast::string_view uri_view) {
    boost::beast::string_view prefix_with_no_trailing_slash = prefix.substr(0, prefix.size() - 1);
    if (uri_view == prefix_with_no_trailing_slash) {
        // Trailing slash edge case (e.g. remove "/api/", but uri is "/api")
        return std::string("");
    }
    uri_view.remove_prefix(prefix.length());
    return std::string(uri_view);
}

int crud_handler::count_path_segments(const std::filesystem::path& path) {
    int count = 0;
    for (const auto& part : path) {
        if (part != "." && part != "..") {
            count++;
        }
    }
    return count;
}

bool crud_handler::has_json_content_type(const http::request<http::string_body>& req) {
    auto it = req.find(http::field::content_type);
    if (it != req.end()) {
        return it->value() == "application/json";
    }
    // Assume not application/json if Content-Type header is missing
    return false;
}

http::response<http::string_body> crud_handler::create_response(http::status status, const std::string& content_type, const std::string& body) {
    http::response<http::string_body> response;
    response.version(11);
    response.result(status);
    response.set(http::field::content_type, content_type);
    if (!body.empty()) {
        response.body() = body;
    }
    response.prepare_payload();
    return response;
}
