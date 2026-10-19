#pragma once

#include "yorq/record.hpp"
#include <filesystem>
#include <vector>

namespace yorq
{
    class WAL
    {
    public:
        explicit WAL(const std::filesystem::path &path);
        ~WAL();

        WAL(const WAL &) = delete;
        WAL &operator=(const WAL &) = delete;

        void append(const Record &record);
        std::vector<Record> recover();

    private:
        int fd_ = -1;
        std::filesystem::path path_;
    }
}