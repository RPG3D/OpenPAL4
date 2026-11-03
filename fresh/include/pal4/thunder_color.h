#pragma once

#include "pal4/thunder_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class ThunderColor final {
public:
    // IDA 0x61c520/0x61c550; owner +44 ChangeColorTask facet.
    [[nodiscard]] static std::uint8_t Get(
        const ThunderObjectState& state,
        std::array<std::uint8_t, 4u>& output);
    [[nodiscard]] static std::uint8_t Set(
        ThunderObjectState& state,
        const std::array<std::uint8_t, 4u>& channels);
};

}  // namespace pal4
