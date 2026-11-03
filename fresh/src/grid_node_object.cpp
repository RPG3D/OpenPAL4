#include "pal4/grid_node_object.h"

namespace pal4 {

std::uint32_t GridNodeObject::Destroy(
    const std::uint32_t address, GridNodeObjectState& state,
    const bool free_self, ISpatialGridNodeRuntime& runtime) {
    (void)RemoveFromGrid(address, state, runtime);
    if (free_self) (void)runtime.Free(address);
    return address;
}

std::uint32_t GridNodeObject::UpdatePosition(
    const std::uint32_t object_address, GridNodeObjectState& state,
    const float x, const float y, const float z,
    ISpatialGridNodeRuntime& runtime) {
    if (state.grid == 0u)
        return 0u;
    return SpatialGrid::UpdateNode(state.grid, object_address + 4u,
                                   x, y, z, runtime);
}

std::uint32_t GridNodeObject::CollectSphere(
    const GridNodeObjectState& state, const float x, const float y,
    const float z, const float radius,
    GridManagerMembersState& collection,
    ISpatialGridNodeRuntime& grid_runtime,
    IGridManagerMembersRuntime& collection_runtime) {
    if (state.grid == 0u)
        return 0u;
    return SpatialGrid::ProcessSphere(
        state.grid, x, y, z, radius,
        [&collection, &collection_runtime](
            const std::uint32_t payload,
            const std::uint32_t distance_bits,
            const std::uint32_t) {
            return GridManagerMembers::AppendVisitedElement(
                collection, payload, distance_bits, collection_runtime);
        },
        0u, grid_runtime);
}

std::uint32_t GridNodeObject::RemoveFromGrid(
    const std::uint32_t object_address, GridNodeObjectState& state,
    ISpatialGridNodeRuntime& runtime) {
    state.vtable = 0x84b6c0u;
    const auto result = SpatialGrid::RemoveNode(object_address + 4u,
                                                 runtime);
    state.vtable = 0x84b6d8u;
    return result;
}

std::uint32_t GridNodeObject::DestroyFacet(
    const std::uint32_t object_address, GridNodeObjectState& state,
    const bool free_self, ISpatialGridNodeRuntime& runtime) {
    state.vtable = 0x84b6d8u;
    if (free_self)
        (void)runtime.Free(object_address);
    return object_address;
}

}  // namespace pal4
