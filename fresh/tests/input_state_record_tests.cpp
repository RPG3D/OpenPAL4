#include "pal4/input_state_record.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <utility>

namespace {

std::uint32_t ReadDword(const pal4::InputStateRecord& record,
                        const std::uint32_t offset) {
    std::uint32_t value = 0u;
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        value |= static_cast<std::uint32_t>(record.Bytes()[offset + index])
                 << (index * 8u);
    }
    return value;
}

}  // namespace

int main() {
    pal4::InputStateRecord record;
    record.Bytes().fill(0xa5u);
    if (&record.Initialize() != &record) {
        std::cerr << "initializer did not return its object\n";
        return EXIT_FAILURE;
    }
    std::array<std::uint8_t, pal4::InputStateRecord::kBytes> expected;
    expected.fill(0xa5u);
    for (const std::uint32_t offset : {0x18u, 0x1cu, 0x28u, 0x2cu}) {
        for (std::uint32_t index = 0u; index < 4u; ++index) {
            expected[offset + index] =
                static_cast<std::uint8_t>(0x7fffffffu >> (index * 8u));
        }
    }
    for (const std::uint32_t offset : {0x10u, 0x14u, 0x20u, 0x24u}) {
        for (std::uint32_t index = 0u; index < 4u; ++index) {
            expected[offset + index] =
                static_cast<std::uint8_t>(0x80000000u >> (index * 8u));
        }
    }
    expected[0x0cu] = 0u;
    expected[0x0du] = 0u;
    expected[0x0eu] = 0x80u;
    expected[0x0fu] = 0x3fu;
    for (const auto [begin, end] : {
             std::pair{0u, 0x0cu}, std::pair{0x38u, 0xf20u},
             std::pair{0x186cu, 0x1878u}}) {
        for (std::uint32_t offset = begin; offset < end; ++offset) {
            expected[offset] = 0u;
        }
    }
    if (record.Bytes() != expected || ReadDword(record, 0x0cu) != 0x3f800000u ||
        ReadDword(record, 0x18u) != 0x7fffffffu ||
        ReadDword(record, 0x20u) != 0x80000000u) {
        std::cerr << "input state writes or preserved gaps differ from IDA\n";
        return EXIT_FAILURE;
    }
    std::cout << "input state initialization layout test passed\n";
}
