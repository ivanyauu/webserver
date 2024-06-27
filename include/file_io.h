#ifndef FILEIO_H
#define FILEIO_H

#include "i_file_io.h"
#include <string>
#include <ios>
#include <fstream>
#include <vector>


class file_io : public i_file_io {
public:
    virtual ~file_io() = default;
    bool open(const std::string& filename, std::ios_base::openmode mode) override;
    bool write(const std::string& data) override;
    void close() override;
    bool read(const std::string& filepath, std::string& content) override;
    bool delete_file(const std::string& filepath) override;
    bool create_directories(const std::string& path) override;
    bool list_directories(const std::string& path,std::vector<std::string>& directories) override;
    bool exists(const std::string& filepath) override;
private:
    std::fstream file_;
};

#endif /* FILEIO_H */
