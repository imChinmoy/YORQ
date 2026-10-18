#include "../../include/yorq/record.hpp"

#include <limits>
#include <stdexcept>
#include <span>
#include <utility>
namespace yorq {

namespace {

constexpr std::uint8_t MAGIC[] = {
    'Y', 'O', 'R', 'Q'
};

constexpr std::uint8_t VERSION = 1;

constexpr std::size_t HEADER_SIZE = 14;
constexpr std::size_t CHECKSUM_SIZE = 4;
constexpr std::size_t MIN_RECORD_SIZE =
    HEADER_SIZE + CHECKSUM_SIZE;

constexpr std::uint32_t MAX_FIELD_SIZE =
    16 * 1024 * 1024;

// Encode a 32-bit integer in little-endian order.
void write_u32(
    std::vector<std::uint8_t>& output,
    std::uint32_t value
) {
    for (int i = 0; i < 4; ++i) {
        output.push_back(
            static_cast<std::uint8_t>(
                (value >> (i * 8)) & 0xFF
            )
        );
    }
}

// Read a 32-bit little-endian integer.
std::uint32_t read_u32(
    std::span<const std::uint8_t> bytes,
    std::size_t offset
) {
    std::uint32_t value = 0;

    for (int i = 0; i < 4; ++i) {
        value |=
            static_cast<std::uint32_t>(bytes[offset + i])
            << (i * 8);
    }

    return value;
}

// CRC-32/ISO-HDLC.
std::uint32_t crc32(
    std::span<const std::uint8_t> bytes
) {
    std::uint32_t crc = 0xFFFFFFFF;

    for (std::uint8_t byte : bytes) {
        crc ^= byte;

        for (int bit = 0; bit < 8; ++bit) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc ^ 0xFFFFFFFF;
}

bool valid_operation(std::uint8_t value) {
    return value ==
               static_cast<std::uint8_t>(Operation::Put)
        || value ==
               static_cast<std::uint8_t>(Operation::Delete);
}

} // namespace

std::vector<std::uint8_t> encode_record(
    const Record& record
) {
    if (record.key.empty()) {
        throw std::invalid_argument(
            "YORQ keys cannot be empty"
        );
    }

    if (record.key.size() > MAX_FIELD_SIZE ||
        record.value.size() > MAX_FIELD_SIZE) {
        throw std::invalid_argument(
            "Record field exceeds maximum size"
        );
    }

    if (record.operation != Operation::Put &&
        record.operation != Operation::Delete) {
        throw std::invalid_argument(
            "Invalid record operation"
        );
    }

    if (record.operation == Operation::Delete &&
        !record.value.empty()) {
        throw std::invalid_argument(
            "Delete records cannot contain a value"
        );
    }

    std::vector<std::uint8_t> bytes;
    bytes.reserve(
        MIN_RECORD_SIZE +
        record.key.size() +
        record.value.size()
    );

    // Header: 14 bytes.
    for (std::uint8_t byte : MAGIC) {
        bytes.push_back(byte);
    }

    bytes.push_back(VERSION);
    bytes.push_back(
        static_cast<std::uint8_t>(record.operation)
    );

    write_u32(
        bytes,
        static_cast<std::uint32_t>(record.key.size())
    );

    write_u32(
        bytes,
        static_cast<std::uint32_t>(record.value.size())
    );

    // Payload.
    for (unsigned char byte : record.key) {
        bytes.push_back(byte);
    }

    for (unsigned char byte : record.value) {
        bytes.push_back(byte);
    }

    // Checksum covers header and payload.
    write_u32(bytes, crc32(bytes));

    return bytes;
}

bool decode_record(
    std::span<const std::uint8_t> bytes,
    Record& output,
    std::size_t& bytes_consumed
) {
    bytes_consumed = 0;

    if (bytes.size() < MIN_RECORD_SIZE) {
        return false;
    }

    // Validate magic.
    for (std::size_t i = 0; i < 4; ++i) {
        if (bytes[i] != MAGIC[i]) {
            return false;
        }
    }

    if (bytes[4] != VERSION ||
        !valid_operation(bytes[5])) {
        return false;
    }

    const auto operation =
        static_cast<Operation>(bytes[5]);

    const std::uint32_t key_length =
        read_u32(bytes, 6);

    const std::uint32_t value_length =
        read_u32(bytes, 10);

    if (key_length == 0 ||
        key_length > MAX_FIELD_SIZE ||
        value_length > MAX_FIELD_SIZE) {
        return false;
    }

    if (operation == Operation::Delete &&
        value_length != 0) {
        return false;
    }

    const std::size_t payload_size =
        static_cast<std::size_t>(key_length) +
        static_cast<std::size_t>(value_length);

    const std::size_t total_size =
        HEADER_SIZE + payload_size + CHECKSUM_SIZE;

    if (bytes.size() < total_size) {
        return false;
    }

    const std::size_t checksum_offset =
        total_size - CHECKSUM_SIZE;

    const std::uint32_t stored_checksum =
        read_u32(bytes, checksum_offset);

    const std::uint32_t calculated_checksum =
        crc32(bytes.first(checksum_offset));

    if (stored_checksum != calculated_checksum) {
        return false;
    }

    // Construct a temporary so failed validation
    // never partially overwrites the output record.
    Record decoded;
    decoded.operation = operation;

    decoded.key.assign(
        reinterpret_cast<const char*>(
            bytes.data() + HEADER_SIZE
        ),
        key_length
    );

    decoded.value.assign(
        reinterpret_cast<const char*>(
            bytes.data() + HEADER_SIZE + key_length
        ),
        value_length
    );

    output = std::move(decoded);
    bytes_consumed = total_size;

    return true;
}

} // namespace yorq
