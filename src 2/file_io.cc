#include "file_io.h"
#include <iterator>
#include <string>
#include <fstream>
#include <filesystem>
#include <vector>


bool file_io::open(const std::string& filename, std::ios_base::openmode mode) {
    file_.open(filename, mode);
    return file_.is_open();
}

bool file_io::write(const std::string& data) {
    if (file_.is_open()) {
        file_ << data;
        return !file_.fail();
    }
    return false;
}

bool file_io::read(const std::string& filepath, std::string& content) {
    
    std::ifstream file(filepath);

    if (!file.is_open()) {
        return false; 
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf(); 
    content = buffer.str();
    file.close();

    return true;
}

bool file_io::delete_file(const std::string& filepath) {
    return std::filesystem::remove(filepath);
}

void file_io::close() {
    if (file_.is_open()) {
        file_.close();
    }
}

bool file_io::create_directories(const std::string& path) {
    return std::filesystem::create_directories(path);
}

bool file_io::list_directories(const std::string& path, std::vector<std::string>& directory) {
    try {
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (entry.is_regular_file()) {
                directory.push_back(entry.path().filename().string());
            }
        }
        return true; 
    } catch (const std::exception& e) {
        return false;
    }
}

bool file_io::exists(const std::string& filepath) {
    return std::filesystem::exists(filepath);
}