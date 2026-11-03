#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct PlayerPropertyState {
    std::array<std::uint32_t, 61> words{};
};
static_assert(sizeof(PlayerPropertyState) == 0xf4u);

class PlayerPropertyValidator final {
public:
    // IDA 0x440350.
    [[nodiscard]] static std::uint32_t Set(
        PlayerPropertyState& state, std::uint32_t index,
        std::int32_t value);
};

}  // namespace pal4
