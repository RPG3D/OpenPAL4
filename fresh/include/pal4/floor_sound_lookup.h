#pragma once
#include <array>
#include <bit>
#include <cstdint>

namespace pal4 {
// Shared non-owning policy recovered from41F9C0/41FB30. Numeric conversion and
// database/string ownership stay with the caller's original/native boundary.
class FloorSoundLookup final {
public:
    static std::array<std::int32_t,4> Candidates(std::int32_t current, std::int32_t previous) {
        const auto bits = std::bit_cast<std::uint32_t>(current);
        return {current, std::bit_cast<std::int32_t>(bits + 1u),
            std::bit_cast<std::int32_t>(bits - 1u), previous};
    }
    static long double ScaleInput(std::int32_t value) {
        return static_cast<long double>(value) *
            static_cast<long double>(0.003921568859368563f) * static_cast<long double>(100.0f);
    }
};
}
