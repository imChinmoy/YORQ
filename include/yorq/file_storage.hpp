#pragma once

#include <string>
#include <cstddef>
#include <filesystem>
#include <fstream>

class FileStorage {
    public : 
    explicit FileStorage(const std::filesystem::path& path);

    bool append(const std::string& data);
    bool read_all(std::string& data);

    void close();

    ~FileStorage();

    private: 
    std::filesystem::path path_;
    std::fstream file_;
};

