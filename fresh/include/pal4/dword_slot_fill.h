#pragma once

#include "pal4/spatial_grid.h"

#include <cstdint>

namespace pal4 {

class DwordSlotFill final {
public:
    [[nodiscard]] static std::uint32_t Fill(
        std::uint32_t destination, std::uint32_t count,
        std::uint32_t value, ISpatialGridRuntime& runtime);
};

}  // namespace pal4
