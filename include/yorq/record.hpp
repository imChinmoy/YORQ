
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace yorq {

enum class Operation : std::uint8_t {
    Put = 1,
    Delete = 2
};

struct Record {
    Operation operation;
    std::string key;
    std::string value;
};

std::vector<std::uint8_t> encode_record(
    const Record& record
);

bool decode_record(
    std::span<const std::uint8_t> bytes,
    Record& output,
    std::size_t& bytes_consumed
);

}
