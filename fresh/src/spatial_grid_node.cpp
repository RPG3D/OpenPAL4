#include "pal4/spatial_grid.h"

#include <bit>

namespace pal4 {

SpatialGridNodeState& SpatialGrid::InitializeNode(
    SpatialGridNodeState& node, const std::uint32_t payload) {
    node.previous = 0u;
    node.next = 0u;
    node.head_slot = 0u;
    node.payload = payload;
    return node;
}

std::uint32_t SpatialGrid::InsertNode(
    const std::uint32_t node_address, const std::uint32_t head_slot,
    ISpatialGridNodeRuntime& runtime) {
    auto& node = runtime.NodeAt(node_address);
    const auto former_head = runtime.ReadWord(head_slot);
    node.previous = 0u;
    node.next = former_head;
    if (former_head != 0u)
        runtime.NodeAt(former_head).previous = node_address;
    runtime.WriteWord(head_slot, node_address);
    node.head_slot = head_slot;
    return head_slot;
}

std::uint32_t SpatialGrid::RemoveNode(
    const std::uint32_t node_address,
    ISpatialGridNodeRuntime& runtime) {
    auto& node = runtime.NodeAt(node_address);
    if (node.head_slot != 0u) {
        if (runtime.ReadWord(node.head_slot) == node_address)
            runtime.WriteWord(node.head_slot, node.next);
        if (node.previous != 0u)
            runtime.NodeAt(node.previous).next = node.next;
        if (node.next != 0u)
            runtime.NodeAt(node.next).previous = node.previous;
    }
    node.previous = 0u;
    node.next = 0u;
    node.head_slot = 0u;
    return node_address;
}

std::uint32_t SpatialGrid::UpdateNode(
    const std::uint32_t grid_address,
    const std::uint32_t node_address,
    const float x, const float y, const float z,
    ISpatialGridNodeRuntime& runtime) {
    const auto slot = BucketAddress(runtime.GridAt(grid_address),
                                    grid_address, x, y, z);
    auto& node = runtime.NodeAt(node_address);
    const auto old_slot = node.head_slot;
    node.position_bits = {std::bit_cast<std::uint32_t>(x),
                          std::bit_cast<std::uint32_t>(y),
                          std::bit_cast<std::uint32_t>(z)};
    if (slot != old_slot) {
        (void)RemoveNode(node_address, runtime);
        return InsertNode(node_address, slot, runtime);
    }
    return old_slot;
}

}  // namespace pal4
