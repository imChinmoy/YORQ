#include "../../include/yorq/file_storage.hpp"

#include <iostream>

FileStorage::FileStorage(const std::filesystem::path& path)
    : path_(path) {
    if (!std::filesystem::exists(path_)) {
        std::ofstream create_file(path_, std::ios::binary);
    }

    file_.open(
        path_,
        std::ios::binary | std::ios::in | std::ios::out
    );

    if (!file_.is_open()) {
        throw std::runtime_error("Failed to open storage file");
    }
}

bool FileStorage::append(const std::string& data) {
    file_.clear();
    file_.seekp(0, std::ios::end);

    file_.write(
        data.data(),
        static_cast<std::streamsize>(data.size())
    );

    file_.flush();

    return file_.good();
}

bool FileStorage::read_all(std::string& output) {
    file_.clear();
    file_.seekg(0, std::ios::beg);

    output.assign(
        std::istreambuf_iterator<char>(file_),
        std::istreambuf_iterator<char>()
    );

    return !file_.bad();
}

void FileStorage::close() {
    if (file_.is_open()) {
        file_.close();
    }
}

FileStorage::~FileStorage() {
    close();
}
