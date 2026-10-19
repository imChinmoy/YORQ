#include "yorq/wal.hpp"

#include <cerrno>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace yorq
{

    namespace
    {

        constexpr std::size_t HEADER_SIZE = 14;
        constexpr std::size_t CHECKSUM_SIZE = 4;
        constexpr std::uint32_t MAX_FIELD_SIZE =
            16 * 1024 * 1024;

        std::uint32_t read_u32(
            const std::vector<std::uint8_t> &bytes,
            std::size_t offset)
        {
            std::uint32_t value = 0;

            for (int i = 0; i < 4; ++i)
            {
                value |=
                    static_cast<std::uint32_t>(bytes[offset + i])
                    << (i * 8);
            }

            return value;
        }

        bool incomplete_tail(
            const std::vector<std::uint8_t> &bytes,
            std::size_t offset)
        {
            const std::size_t remaining = bytes.size() - offset;

            if (remaining < HEADER_SIZE)
            {
                return true;
            }

            const auto *p = bytes.data() + offset;

            if (p[0] != 'Y' || p[1] != 'O' ||
                p[2] != 'R' || p[3] != 'Q' ||
                p[4] != 1 ||
                (p[5] != 1 && p[5] != 2))
            {
                return false;
            }

            const std::uint32_t key_size =
                read_u32(bytes, offset + 6);

            const std::uint32_t value_size =
                read_u32(bytes, offset + 10);

            if (key_size == 0 ||
                key_size > MAX_FIELD_SIZE ||
                value_size > MAX_FIELD_SIZE)
            {
                return false;
            }

            if (p[5] == 2 && value_size != 0)
            {
                return false;
            }

            const std::size_t expected =
                HEADER_SIZE +
                static_cast<std::size_t>(key_size) +
                static_cast<std::size_t>(value_size) +
                CHECKSUM_SIZE;

            return remaining < expected;
        }

        void write_all(int fd, const std::uint8_t *data,
                       std::size_t size)
        {
            std::size_t written = 0;

            while (written < size)
            {
                const ssize_t result = ::write(
                    fd, data + written, size - written);

                if (result < 0)
                {
                    if (errno == EINTR)
                    {
                        continue;
                    }

                    throw std::system_error(
                        errno, std::generic_category(),
                        "WAL write failed");
                }

                if (result == 0)
                {
                    throw std::runtime_error(
                        "WAL write made no progress");
                }

                written += static_cast<std::size_t>(result);
            }
        }

    } // namespace

    WAL::WAL(const std::filesystem::path &path)
        : path_(path)
    {
        fd_ = ::open(
            path_.c_str(),
            O_CREAT | O_RDWR | O_APPEND | O_CLOEXEC,
            0644);

        if (fd_ < 0)
        {
            throw std::system_error(
                errno, std::generic_category(),
                "Failed to open WAL");
        }
    }

    WAL::~WAL()
    {
        if (fd_ >= 0)
        {
            ::close(fd_);
        }
    }

    void WAL::append(const Record &record)
    {
        const auto bytes = encode_record(record);

        write_all(fd_, bytes.data(), bytes.size());

        if (::fdatasync(fd_) != 0)
        {
            throw std::system_error(
                errno, std::generic_category(),
                "WAL synchronization failed");
        }
    }

    std::vector<Record> WAL::recover()
    {
        struct stat info{};

        if (::fstat(fd_, &info) != 0)
        {
            throw std::system_error(
                errno, std::generic_category(),
                "Failed to inspect WAL");
        }

        if (info.st_size < 0)
        {
            throw std::runtime_error("Invalid WAL size");
        }

        std::vector<std::uint8_t> bytes(
            static_cast<std::size_t>(info.st_size));

        std::size_t loaded = 0;

        while (loaded < bytes.size())
        {
            const ssize_t result = ::pread(
                fd_,
                bytes.data() + loaded,
                bytes.size() - loaded,
                static_cast<off_t>(loaded));

            if (result < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }

                throw std::system_error(
                    errno, std::generic_category(),
                    "Failed to read WAL");
            }

            if (result == 0)
            {
                throw std::runtime_error(
                    "WAL changed during recovery");
            }

            loaded += static_cast<std::size_t>(result);
        }

        std::vector<Record> records;
        std::size_t offset = 0;

        while (offset < bytes.size())
        {
            Record record{};
            std::size_t consumed = 0;

            const auto remaining =
                std::span<const std::uint8_t>(bytes)
                    .subspan(offset);

            if (!decode_record(
                    remaining, record, consumed))
            {
                if (incomplete_tail(bytes, offset))
                {
                    // Discard only the incomplete EOF
                    if (::ftruncate(
                            fd_,
                            static_cast<off_t>(offset)) != 0)
                    {
                        throw std::system_error(
                            errno, std::generic_category(),
                            "Failed to truncate incomplete WAL tail");
                    }

                    if (::fdatasync(fd_) != 0)
                    {
                        throw std::system_error(
                            errno, std::generic_category(),
                            "Failed to sync repaired WAL");
                    }

                    break;
                }

                throw std::runtime_error(
                    "WAL corruption at byte " +
                    std::to_string(offset));
            }

            records.push_back(std::move(record));
            offset += consumed;
        }

        return records;
    }

}
