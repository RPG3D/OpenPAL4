#pragma once

#include <cstdint>

namespace pal4 {

class ITimerFacetRuntime {
public:
    virtual ~ITimerFacetRuntime() = default;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

// 0x47D140 uses a different, game-specific vtable. Preserve this adapter;
// it is not evidence of a replaceable std::locale private implementation.
class TimerFacetLifecycle final {
public:
    [[nodiscard]] static std::uint32_t DestroyWithFlag(
        std::uint32_t facet, std::uint8_t flags, ITimerFacetRuntime& runtime);
};

}  // namespace pal4
