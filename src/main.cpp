#include "yorq/file_storage.hpp"
#include "yorq/record.hpp"

#include <cstdint>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>

int main()
{
    try
    {
        FileStorage storage("data.yorq");

        const yorq::Record records[] = {
            {yorq::Operation::Put, "username", "chinmoy"},
            {yorq::Operation::Put, "theme", "dark"},
            {yorq::Operation::Put, "language", "cpp"}};

        // encode & append
        for (const auto &record : records)
        {
            const auto encoded =
                yorq::encode_record(record);

            const std::string binary_data(
                reinterpret_cast<const char *>(
                    encoded.data()),
                encoded.size());

            if (!storage.append(binary_data))
            {
                throw std::runtime_error(
                    "Failed to append record");
            }
        }

        std::string contents;

        if (!storage.read_all(contents))
        {
            throw std::runtime_error(
                "Failed to read storage file");
        }

        // file to byte span
        const auto *raw =
            reinterpret_cast<const std::uint8_t *>(
                contents.data());

        const std::span<const std::uint8_t> bytes(
            raw,
            contents.size());

        // Decode
        std::size_t offset = 0;
        std::size_t record_count = 0;

        while (offset < bytes.size())
        {
            yorq::Record decoded{};
            std::size_t consumed = 0;

            if (!yorq::decode_record(
                    bytes.subspan(offset),
                    decoded,
                    consumed))
            {
                std::cerr
                    << "Invalid or corrupted record at byte "
                    << offset << '\n';

                return 1;
            }

            if (consumed == 0)
            {
                throw std::runtime_error(
                    "Decoder consumed zero bytes");
            }

            std::cout
                << decoded.key << " = "
                << decoded.value << '\n';

            offset += consumed;
            ++record_count;
        }

        std::cout
            << "Successfully decoded "
            << record_count << " records.\n";

        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "YORQ error: "
                  << error.what() << '\n';

        return 1;
    }
}
