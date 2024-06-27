#ifndef IFILEIO_H
#define IFILEIO_H

#include <string>
#include <vector>
#include <ios>

class i_file_io {
public:
    virtual ~i_file_io() = default;
    virtual bool open(const std::string& filename, std::ios_base::openmode mode) = 0;
    virtual bool write(const std::string& data) = 0;
    virtual void close() = 0;
    virtual bool read(const std::string& filepath, std::string& content) = 0;
    virtual bool delete_file(const std::string& filepath) = 0;
    virtual bool create_directories(const std::string& path) = 0;
    virtual bool list_directories(const std::string& path,std::vector<std::string>& directories) = 0;
    virtual bool exists(const std::string& filepath) = 0;
};

#endif /* IFILEIO_H */
