#pragma once

#include <cstdint>
#include <exception>

struct RpClump;

namespace pal4 {

class NativeRwPlugin8452Processor final {
public:
    [[nodiscard]] static std::uint32_t ProcessClump(RpClump* clump);
    // IDA 0x66CE70: game visibility/collision admission, not the SDK world query.
    [[nodiscard]] static bool QueryClump(RpClump* clump);
    static void RestoreClump(RpClump* clump);
    [[nodiscard]] static std::uint32_t RenderCallbackAddress();
    [[nodiscard]] static std::exception_ptr TakeRenderError();
};

}  // namespace pal4
