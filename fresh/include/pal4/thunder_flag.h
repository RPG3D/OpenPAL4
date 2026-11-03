#pragma once

#include "pal4/thunder_object.h"

#include <cstdint>

namespace pal4 {

class ThunderFlag final {
public:
    // IDA 0x61cb20/0x61cb30: +48 facet, owner byte +62.
    [[nodiscard]] static std::uint8_t Get(const ThunderObjectState& state);
    [[nodiscard]] static std::uint8_t Set(ThunderObjectState& state,
                                          std::uint8_t value);
};

}  // namespace pal4
