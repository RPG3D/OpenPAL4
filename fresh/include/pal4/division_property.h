#pragma once

#include <cstdint>

namespace pal4 {

struct DivisionPropertyState {
    std::uint32_t header[2]{};
    float inverse_divisor{};
    std::uint32_t count{};
};
static_assert(sizeof(DivisionPropertyState) == 16u);

class DivisionProperty final {
public:
    // IDA 0x6492a0.
    [[nodiscard]] static std::uint32_t SetDivisor(
        DivisionPropertyState& state, std::uint32_t divisor);
    // IDA 0x6492e0.
    [[nodiscard]] static std::uint8_t SetFlag(
        DivisionPropertyState& state, std::uint8_t flag);
};

}  // namespace pal4
