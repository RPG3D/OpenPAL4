#pragma once

#include "pal4/strip_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IStripColorRuntime {
public:
    virtual ~IStripColorRuntime() = default;
    [[nodiscard]] virtual std::uint8_t SetClumpMaterialColor(
        std::uint32_t clump, std::uint32_t rgba) = 0;
};

class StripColor final {
public:
    // IDA 0x61a0f0/0x61a120, owner facet +44.
    [[nodiscard]] static std::uint8_t Get(
        const StripObjectState& state, std::array<std::uint8_t, 4>& output);
    [[nodiscard]] static std::uint8_t Set(
        StripObjectState& state, const std::array<std::uint8_t, 4>& rgba,
        IStripColorRuntime& runtime);
};

}  // namespace pal4
