#include "pal4/spatial_grid.h"

#include <bit>
#include <cmath>
#include <limits>

namespace pal4 {

std::uint32_t SpatialGrid::Create(
    const SpatialGridConfig& config, ISpatialGridRuntime& runtime) {
    const auto address = runtime.Allocate(sizeof(SpatialGridState));
    if (address == 0u)
        return 0u;
    auto& state = runtime.GridAt(address);
    const auto count = Initialize(state, config, runtime);
    if (count != 0u && state.buckets == 0u) {
        (void)runtime.Free(address);
        return 0u;
    }
    return address;
}

std::uint32_t SpatialGrid::Destroy(
    const std::uint32_t address, ISpatialGridRuntime& runtime) {
    const auto buckets = runtime.GridAt(address).buckets;
    (void)runtime.Free(buckets);
    return runtime.Free(address);
}

std::uint32_t SpatialGrid::Initialize(
    SpatialGridState& state, const SpatialGridConfig& config,
    ISpatialGridRuntime& runtime) {
    for (std::size_t axis = 0; axis < 3u; ++axis) {
        state.origin_bits[axis] =
            std::bit_cast<std::uint32_t>(config.origin[axis]);
        state.extent_bits[axis] =
            std::bit_cast<std::uint32_t>(config.extent[axis]);
        state.dimensions[axis] = config.dimensions[axis];
    }
    const auto count = config.dimensions[0] * config.dimensions[1] *
                       config.dimensions[2];
    state.buckets = runtime.Allocate(count * 4u);
    if (state.buckets != 0u) {
        for (std::uint32_t index = 0; index < count; ++index)
            runtime.WriteWord(state.buckets + 4u * index, 0u);
    }
    state.overflow_head = 0u;
    return count;
}

std::uint32_t SpatialGrid::BucketAddress(
    const SpatialGridState& state, const std::uint32_t grid_address,
    const float x, const float y, const float z) {
    const std::array<float, 3u> position{x, y, z};
    std::array<std::uint32_t, 3u> index{};
    for (std::size_t axis = 0; axis < 3u; ++axis) {
        const auto origin = std::bit_cast<float>(state.origin_bits[axis]);
        const auto extent = std::bit_cast<float>(state.extent_bits[axis]);
        if (!(position[axis] >= origin &&
              position[axis] < origin + extent) ||
            extent == 0.0f || state.dimensions[axis] == 0u)
            return grid_address + 40u;
        const auto cell = static_cast<double>(position[axis] - origin) /
                          static_cast<double>(extent) *
                          static_cast<double>(state.dimensions[axis]);
        if (!std::isfinite(cell) || cell < 0.0 ||
            cell >= static_cast<double>(state.dimensions[axis]))
            return grid_address + 40u;
        index[axis] = static_cast<std::uint32_t>(cell);
    }
    const auto flat = ((index[0] * state.dimensions[1]) + index[1]) *
                          state.dimensions[2] + index[2];
    return state.buckets + flat * 4u;
}

}  // namespace pal4
