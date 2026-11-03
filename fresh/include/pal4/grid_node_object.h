#pragma once

#include "pal4/grid_manager_members.h"
#include "pal4/spatial_grid.h"

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct GridNodeObjectState final {
    std::uint32_t vtable = 0u;
    SpatialGridNodeState node{};
    std::uint32_t grid = 0u;
};
static_assert(sizeof(GridNodeObjectState) == 36u);
static_assert(offsetof(GridNodeObjectState, node) == 4u);
static_assert(offsetof(GridNodeObjectState, grid) == 32u);

class GridNodeObject final {
public:
    // IDA 0x65E9B0: unlink before optionally freeing the association.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object_address, GridNodeObjectState& state,
        bool free_self, ISpatialGridNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdatePosition(
        std::uint32_t object_address, GridNodeObjectState& state,
        float x, float y, float z, ISpatialGridNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t CollectSphere(
        const GridNodeObjectState& state, float x, float y, float z,
        float radius, GridManagerMembersState& collection,
        ISpatialGridNodeRuntime& grid_runtime,
        IGridManagerMembersRuntime& collection_runtime);
    [[nodiscard]] static std::uint32_t RemoveFromGrid(
        std::uint32_t object_address, GridNodeObjectState& state,
        ISpatialGridNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyFacet(
        std::uint32_t object_address, GridNodeObjectState& state,
        bool free_self, ISpatialGridNodeRuntime& runtime);
};

}  // namespace pal4
