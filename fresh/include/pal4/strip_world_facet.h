#pragma once

#include "pal4/strip_object.h"

#include <cstdint>

namespace pal4 {

class IStripWorldFacetRuntime {
public:
    virtual ~IStripWorldFacetRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ResolveWorld(
        std::uint32_t clump) = 0;
    [[nodiscard]] virtual std::uint8_t RemoveClump(
        std::uint32_t world, std::uint32_t clump) = 0;
};

class StripWorldFacet final {
public:
    // IDA 0x619ca0; this points to the facet at owner +20.
    [[nodiscard]] static std::uint8_t Remove(
        StripObjectState& state, IStripWorldFacetRuntime& runtime);
};

}  // namespace pal4
