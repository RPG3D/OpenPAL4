#include "pal4/spatial_grid.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>

namespace pal4 {
namespace {

float Coordinate(const std::uint32_t bits) {
    return std::bit_cast<float>(bits);
}

std::uint32_t VisitSphereChain(
    std::uint32_t node_address, const float x, const float y, const float z,
    const float radius_squared, SpatialGrid::Visitor visitor,
    const std::uint32_t context, ISpatialGridNodeRuntime& runtime,
    std::uint32_t result) {
    while (node_address != 0u) {
        const auto& node = runtime.NodeAt(node_address);
        const double dx = static_cast<double>(x) - Coordinate(node.position_bits[0]);
        const double dy = static_cast<double>(y) - Coordinate(node.position_bits[1]);
        const double dz = static_cast<double>(z) - Coordinate(node.position_bits[2]);
        const double distance_squared = dz * dz + dy * dy + dx * dx;
        if (distance_squared < radius_squared) {
            result = visitor(node.payload,
                             std::bit_cast<std::uint32_t>(
                                 static_cast<float>(distance_squared)),
                             context);
        }
        node_address = node.next;
    }
    return result;
}

int CellIndex(const float position, const float origin, const float extent,
              const std::uint32_t count) {
    const double index = std::trunc(
        (static_cast<double>(position) - origin) / extent * count);
    if (index < std::numeric_limits<int>::min())
        return std::numeric_limits<int>::min();
    if (index > std::numeric_limits<int>::max())
        return std::numeric_limits<int>::max();
    return static_cast<int>(index);
}

}  // namespace

std::uint32_t SpatialGrid::ProcessSphereCells(
    const std::uint32_t grid_address, const float x, const float y,
    const float z, const float radius, const Visitor visitor,
    const std::uint32_t context, const std::array<int, 3u>& first,
    const std::array<int, 3u>& last, ISpatialGridNodeRuntime& runtime) {
    if (first[0] > last[0])
        return static_cast<std::uint32_t>(last[0]);
    const auto& grid = runtime.GridAt(grid_address);
    const auto squared = radius * radius;
    for (int ix = first[0]; ix <= last[0]; ++ix) {
        for (int iy = first[1]; iy <= last[1]; ++iy) {
            for (int iz = first[2]; iz <= last[2]; ++iz) {
                const auto offset = (static_cast<std::uint64_t>(ix) *
                                     grid.dimensions[1] + iy) *
                                        grid.dimensions[2] + iz;
                (void)VisitSphereChain(runtime.ReadWord(
                    grid.buckets + static_cast<std::uint32_t>(4u * offset)),
                    x, y, z, squared, visitor, context, runtime, 0u);
            }
        }
    }
    return 0u;
}

std::uint32_t SpatialGrid::ProcessSphereOverflow(
    const std::uint32_t grid_address, const float x, const float y,
    const float z, const float radius, const Visitor visitor,
    const std::uint32_t context, ISpatialGridNodeRuntime& runtime) {
    const auto& grid = runtime.GridAt(grid_address);
    return VisitSphereChain(grid.overflow_head, x, y, z, radius * radius,
                            visitor, context, runtime, grid_address);
}

std::uint32_t SpatialGrid::ProcessSphere(
    const std::uint32_t grid_address, const float x, const float y,
    const float z, const float radius, const Visitor visitor,
    const std::uint32_t context, ISpatialGridNodeRuntime& runtime) {
    const auto& grid = runtime.GridAt(grid_address);
    const std::array<float, 3u> center{x, y, z};
    std::array<int, 3u> first{};
    std::array<int, 3u> last{};
    bool crosses_boundary = false;
    for (std::size_t axis = 0u; axis < 3u; ++axis) {
        const float origin = Coordinate(grid.origin_bits[axis]);
        const float extent = Coordinate(grid.extent_bits[axis]);
        if (!(extent > 0.0f) || !std::isfinite(origin) ||
            !std::isfinite(extent) || grid.dimensions[axis] == 0u ||
            !std::isfinite(center[axis]) || !std::isfinite(radius))
            return ProcessSphereOverflow(grid_address, x, y, z, radius,
                                         visitor, context, runtime);
        if (center[axis] + radius < origin ||
            center[axis] - radius >= origin + extent)
            return ProcessSphereOverflow(grid_address, x, y, z, radius,
                                         visitor, context, runtime);
        first[axis] = CellIndex(center[axis] - radius, origin, extent,
                                grid.dimensions[axis]);
        last[axis] = CellIndex(center[axis] + radius, origin, extent,
                               grid.dimensions[axis]);
        if (first[axis] < 0 ||
            last[axis] >= static_cast<int>(grid.dimensions[axis]))
            crosses_boundary = true;
        first[axis] = std::max(0, first[axis]);
        last[axis] = std::min(static_cast<int>(grid.dimensions[axis]) - 1,
                              last[axis]);
    }
    if (crosses_boundary)
        (void)ProcessSphereOverflow(grid_address, x, y, z, radius,
                                    visitor, context, runtime);
    return ProcessSphereCells(grid_address, x, y, z, radius, visitor,
                              context, first, last, runtime);
}

std::uint32_t SpatialGrid::TraverseNodes(
    std::uint32_t first_node, const Visitor visitor,
    const std::uint32_t context, ISpatialGridNodeRuntime& runtime) {
    std::uint32_t result = 0u;
    while (first_node != 0u) {
        const auto& node = runtime.NodeAt(first_node);
        result = visitor(node.payload, 0u, context);
        first_node = node.next;
    }
    return result;
}

std::uint32_t SpatialGrid::ProcessAllNodes(
    const std::uint32_t grid_address, const Visitor visitor,
    const std::uint32_t context, ISpatialGridNodeRuntime& runtime) {
    const auto& grid = runtime.GridAt(grid_address);
    const auto count = grid.dimensions[0] * grid.dimensions[1] *
                       grid.dimensions[2];
    for (std::uint32_t index = 0u; index < count; ++index)
        (void)TraverseNodes(runtime.ReadWord(grid.buckets + 4u * index),
                            visitor, context, runtime);
    return TraverseNodes(grid.overflow_head, visitor, context, runtime);
}

}  // namespace pal4
