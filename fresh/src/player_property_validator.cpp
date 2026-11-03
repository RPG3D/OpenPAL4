#include "pal4/player_property_validator.h"

#include <bit>

namespace pal4 {

std::uint32_t PlayerPropertyValidator::Set(
    PlayerPropertyState& state, std::uint32_t index,
    std::int32_t value) {
    if (index >= 10u)
        return index;

    auto& property = state.words[51u + index];
    property = std::bit_cast<std::uint32_t>(value);
    if (index == 7u || index == 9u) {
        const auto maximum = std::bit_cast<std::int32_t>(
            state.words[index == 7u ? 1u : 3u]);
        if (value > maximum)
            property = std::bit_cast<std::uint32_t>(maximum);
        else if (value < 0)
            property = 0u;
    } else if (index == 8u) {
        if (value > 100)
            property = 100u;
        else if (value < 0)
            property = 0u;
    }
    return property;
}

}  // namespace pal4
