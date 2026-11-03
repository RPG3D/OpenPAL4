#include "pal4/pathway_grid.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>

namespace pal4 {
namespace {

constexpr std::uint32_t kPrimaryVtable = 0x84b5e4u;
constexpr std::uint32_t kVirtualBaseTable = 0x84b5f8u;
constexpr std::uint32_t kSecondaryVtable = 0x84b5d4u;

std::uint32_t Bits(const float value) {
    return std::bit_cast<std::uint32_t>(value);
}

void WritePoint(IPathwayGridRuntime& runtime, const std::uint32_t base,
                const std::uint32_t index, const PathwayPoint& point) {
    for (std::uint32_t axis = 0u; axis < 3u; ++axis)
        runtime.WriteWord(base + 12u * index + 4u * axis,
                          Bits(point.coordinates[axis]));
}

}  // namespace

PathwayGridState& PathwayGrid::Construct(
    PathwayGridState& state, const bool initialize_virtual_base) {
    if (initialize_virtual_base)
        state.vbtable = kVirtualBaseTable;
    state.primary_vtable = kPrimaryVtable;
    state.secondary_vtable = kSecondaryVtable;
    state.offset_to_top = 0u;
    state.point_count = 0u;
    state.positions = 0u;
    state.lengths = 0u;
    state.directions = 0u;
    state.segment_length_bits = 0u;
    state.projection_bits = 0u;
    state.difference_bits = {};
    state.nearest_bits = {};
    state.direction_bits = {};
    return state;
}

std::uint32_t PathwayGrid::Destroy(
    PathwayGridState& state, IPathwayGridRuntime& runtime) {
    state.primary_vtable = kPrimaryVtable;
    state.secondary_vtable = kSecondaryVtable;
    state.offset_to_top = 0u;
    const auto result = state.directions;
    if (state.lengths != 0u)
        runtime.Free(state.lengths);
    if (state.positions != 0u)
        runtime.Free(state.positions);
    if (state.directions != 0u)
        runtime.Free(state.directions);
    state.lengths = 0u;
    state.positions = 0u;
    state.directions = 0u;
    return result;
}

bool PathwayGrid::Initialize(
    PathwayGridState& state, const std::span<const PathwayPoint> points,
    const std::uint32_t kind, const bool loop,
    IPathwayGridRuntime& runtime) {
    (void)Destroy(state, runtime);
    if (points.size() > std::numeric_limits<std::uint32_t>::max() -
                            static_cast<std::uint32_t>(loop))
        return false;
    const auto count = static_cast<std::uint32_t>(points.size()) +
                       static_cast<std::uint32_t>(loop);
    if (count == 0u || count > std::numeric_limits<std::uint32_t>::max() / 12u ||
        (loop && points.empty()))
        return false;
    state.point_count = count;
    state.kind = kind;
    state.loop_word = (state.loop_word & 0xffffff00u) |
                      static_cast<std::uint32_t>(loop);
    state.total_length_bits = 0u;
    state.lengths = runtime.Allocate(4u * count);
    state.positions = runtime.Allocate(12u * count);
    state.directions = runtime.Allocate(12u * count);
    if (state.lengths == 0u || state.positions == 0u ||
        state.directions == 0u) {
        (void)Destroy(state, runtime);
        state.point_count = 0u;
        return false;
    }
    for (std::uint32_t index = 0u; index < count; ++index) {
        const auto& point = points[index == points.size() ? 0u : index];
        WritePoint(runtime, state.positions, index, point);
        runtime.WriteWord(state.lengths + 4u * index, 0u);
        for (std::uint32_t axis = 0u; axis < 3u; ++axis)
            runtime.WriteWord(state.directions + 12u * index + 4u * axis, 0u);
    }
    float total = 0.0f;
    for (std::uint32_t index = 1u; index < count; ++index) {
        const auto& current = points[index == points.size() ? 0u : index];
        const auto& previous = points[index - 1u];
        std::array<float, 3u> delta{};
        double squared = 0.0;
        for (std::uint32_t axis = 0u; axis < 3u; ++axis) {
            delta[axis] = current.coordinates[axis] -
                          previous.coordinates[axis];
            squared += static_cast<double>(delta[axis]) * delta[axis];
        }
        const float length = static_cast<float>(std::sqrt(squared));
        runtime.WriteWord(state.lengths + 4u * index, Bits(length));
        for (std::uint32_t axis = 0u; axis < 3u; ++axis)
            runtime.WriteWord(state.directions + 12u * index + 4u * axis,
                              Bits(length == 0.0f ? 0.0f : delta[axis] / length));
        total += length;
    }
    state.total_length_bits = Bits(total);
    return true;
}

}  // namespace pal4
