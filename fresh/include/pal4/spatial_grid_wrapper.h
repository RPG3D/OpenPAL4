#pragma once

#include "pal4/spatial_grid.h"

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct SpatialGridWrapperState final {
    std::uint32_t vtable = 0u;
    std::uint32_t grid = 0u;
};
static_assert(sizeof(SpatialGridWrapperState) == 8u);
static_assert(offsetof(SpatialGridWrapperState, grid) == 4u);

class SpatialGridWrapper final {
public:
    // IDA 0x65E860: allocate a 36-byte association, preserving position bytes.
    [[nodiscard]] static std::uint32_t CreateNode(
        const SpatialGridWrapperState& state, std::uint32_t payload,
        ISpatialGridNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t CountNodes(
        const SpatialGridWrapperState& state,
        ISpatialGridNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t IncrementCount(
        std::uint32_t& count, std::uint32_t payload,
        std::uint32_t distance_bits);
    [[nodiscard]] static std::uint32_t DestroyBase(
        SpatialGridWrapperState& state, ISpatialGridNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t wrapper_address, SpatialGridWrapperState& state,
        bool free_self, ISpatialGridNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyFacet(
        std::uint32_t wrapper_address, SpatialGridWrapperState& state,
        bool free_self, ISpatialGridNodeRuntime& runtime);
};

}  // namespace pal4
