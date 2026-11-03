#include "pal4/spatial_grid_wrapper.h"

#include <cstdint>

namespace pal4 {
namespace {

constexpr std::uint32_t kWrapperVtable = 0x84b6b4u;
constexpr std::uint32_t kFacetVtable = 0x73bbf3u;

}  // namespace

std::uint32_t SpatialGridWrapper::CreateNode(
    const SpatialGridWrapperState& state, const std::uint32_t payload,
    ISpatialGridNodeRuntime& runtime) {
    const auto object = runtime.Allocate(36u);
    if (object == 0u) return 0u;
    runtime.WriteWord(object, 0x84b6c0u);
    (void)SpatialGrid::InitializeNode(runtime.NodeAt(object + 4u), payload);
    runtime.WriteWord(object + 32u, state.grid);
    return object;
}

std::uint32_t SpatialGridWrapper::IncrementCount(
    std::uint32_t& count, const std::uint32_t,
    const std::uint32_t) {
    ++count;
    return count;
}

std::uint32_t SpatialGridWrapper::CountNodes(
    const SpatialGridWrapperState& state,
    ISpatialGridNodeRuntime& runtime) {
    if (state.grid == 0u)
        return 0u;
    const auto& grid = runtime.GridAt(state.grid);
    std::uint32_t count = 0u;
    const auto bucket_count = grid.dimensions[0] * grid.dimensions[1] *
                              grid.dimensions[2];
    for (std::uint32_t index = 0u; index <= bucket_count; ++index) {
        const auto first = index == bucket_count ? grid.overflow_head :
            runtime.ReadWord(grid.buckets + 4u * index);
        for (auto node = first; node != 0u; node = runtime.NodeAt(node).next)
            (void)IncrementCount(count, runtime.NodeAt(node).payload, 0u);
    }
    return count;
}

std::uint32_t SpatialGridWrapper::DestroyBase(
    SpatialGridWrapperState& state, ISpatialGridNodeRuntime& runtime) {
    state.vtable = kWrapperVtable;
    const auto result = state.grid == 0u ? 0u :
        SpatialGrid::Destroy(state.grid, runtime);
    state.grid = 0u;
    state.vtable = kFacetVtable;
    return result;
}

std::uint32_t SpatialGridWrapper::Destroy(
    const std::uint32_t wrapper_address, SpatialGridWrapperState& state,
    const bool free_self, ISpatialGridNodeRuntime& runtime) {
    (void)DestroyBase(state, runtime);
    if (free_self)
        (void)runtime.Free(wrapper_address);
    return wrapper_address;
}

std::uint32_t SpatialGridWrapper::DestroyFacet(
    const std::uint32_t wrapper_address, SpatialGridWrapperState& state,
    const bool free_self, ISpatialGridNodeRuntime& runtime) {
    state.vtable = kFacetVtable;
    if (free_self)
        (void)runtime.Free(wrapper_address);
    return wrapper_address;
}

}  // namespace pal4
