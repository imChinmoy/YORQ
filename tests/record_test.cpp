#include "../include/yorq/record.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    const yorq::Record original{
        yorq::Operation::Put,
        "username",
        "chinmoy"
    };

    const auto encoded = yorq::encode_record(original);

    yorq::Record decoded{};
    std::size_t bytes_consumed = 0;

    assert(yorq::decode_record(
        encoded, decoded, bytes_consumed
    ));

    assert(decoded.operation == original.operation);
    assert(decoded.key == original.key);
    assert(decoded.value == original.value);
    assert(bytes_consumed == encoded.size());

    // A damaged record must be rejected.
    auto corrupted = encoded;
    corrupted[14] ^= 0x01;

    yorq::Record rejected{};
    std::size_t rejected_size = 0;

    assert(!yorq::decode_record(
        corrupted, rejected, rejected_size
    ));

    // Empty values are valid.
    const yorq::Record empty_value{
        yorq::Operation::Put,
        "theme",
        ""
    };

    const auto empty_encoded =
        yorq::encode_record(empty_value);

    yorq::Record empty_decoded{};
    std::size_t empty_size = 0;

    assert(yorq::decode_record(
        empty_encoded, empty_decoded, empty_size
    ));

    assert(empty_decoded.value.empty());

    std::cout << "All record tests passed!\n";
}
