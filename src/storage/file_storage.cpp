
#include "yorq/file_storage.hpp"

#include <iterator>
#include <limits>
#include <stdexcept>

FileStorage::FileStorage(const std::filesystem::path& path)
    : path_(path) {
    if (!std::filesystem::exists(path_)) {
        std::ofstream create_file(
            path_,
            std::ios::binary
        );

        if (!create_file) {
            throw std::runtime_error(
                "Failed to create storage file"
            );
        }
    }

    file_.open(
        path_,
        std::ios::binary |
        std::ios::in |
        std::ios::out
    );

    if (!file_.is_open()) {
        throw std::runtime_error(
            "Failed to open storage file"
        );
    }
}

bool FileStorage::append(const std::string& data) {
    if (!file_.is_open()) {
        return false;
    }

    if (data.size() >
        static_cast<std::size_t>(
            std::numeric_limits<std::streamsize>::max()
        )) {
        return false;
    }

    file_.clear();
    file_.seekp(0, std::ios::end);

    if (!file_) {
        return false;
    }

    file_.write(
        data.data(),
        static_cast<std::streamsize>(data.size())
    );

    file_.flush();

    return file_.good();
}

bool FileStorage::read_all(std::string& output) {
    if (!file_.is_open()) {
        return false;
    }

    file_.clear();
    file_.seekg(0, std::ios::beg);

    if (!file_) {
        return false;
    }

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
