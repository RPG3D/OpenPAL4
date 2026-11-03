#pragma once

#include "pal4/world_sector_lightmap_pipeline.h"

#include <cstdint>

namespace pal4 {

class MatFxWorldSectorRenderer final {
public:
    static void Render(
        std::uint32_t render_data, std::uint32_t clipping,
        std::uint32_t clipping_flags, std::uint32_t render_flags,
        IWorldSectorRenderRuntime& runtime);
};

}  // namespace pal4
