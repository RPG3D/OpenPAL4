#pragma once

#include <cstdint>

struct RpClump;

namespace pal4 {

class NativeArenaWorldClumpBinding final {
public:
    [[nodiscard]] static bool Attach(std::uint32_t arena, RpClump* clump);
    [[nodiscard]] static bool Detach(std::uint32_t arena, RpClump* clump);
};

}  // namespace pal4
