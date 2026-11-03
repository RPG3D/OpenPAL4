#include "pal4/spatial_grid.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

class Runtime final : public pal4::ISpatialGridNodeRuntime {
public:
    std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    std::uint32_t Free(std::uint32_t) override { return 0u; }
    pal4::SpatialGridState& GridAt(std::uint32_t address) override {
        Check(address == 0x1000u, "grid node uses expected grid address");
        return grid;
    }
    pal4::SpatialGridNodeState& NodeAt(
        std::uint32_t address) override {
        Check(address == 0x3000u || address == 0x3020u ||
                  address == 0x3040u,
              "node lookup uses known allocation");
        return nodes[address];
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return heads[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        heads[address] = value;
    }

    pal4::SpatialGridState grid{};
    std::map<std::uint32_t, pal4::SpatialGridNodeState> nodes;
    std::map<std::uint32_t, std::uint32_t> heads;
};

void InitializeAndList() {
    Runtime runtime;
    auto& first = runtime.nodes[0x3000u];
    first.position_bits = {1u, 2u, 3u};
    first.previous = 9u;
    first.next = 8u;
    first.head_slot = 7u;
    Check(&pal4::SpatialGrid::InitializeNode(first, 0x77u) == &first &&
              first.previous == 0u && first.next == 0u &&
              first.head_slot == 0u && first.payload == 0x77u &&
              first.position_bits ==
                  std::array<std::uint32_t, 3u>{1u, 2u, 3u},
          "node initializer resets links and preserves position words");
    (void)pal4::SpatialGrid::InitializeNode(
        runtime.nodes[0x3020u], 0x88u);
    (void)pal4::SpatialGrid::InitializeNode(
        runtime.nodes[0x3040u], 0x99u);
    Check(pal4::SpatialGrid::InsertNode(
              0x3000u, 0x2000u, runtime) == 0x2000u &&
              runtime.heads[0x2000u] == 0x3000u &&
              first.previous == 0u && first.next == 0u &&
              first.head_slot == 0x2000u,
          "insert into empty bucket becomes sole head");
    (void)pal4::SpatialGrid::InsertNode(0x3020u, 0x2000u, runtime);
    (void)pal4::SpatialGrid::InsertNode(0x3040u, 0x2000u, runtime);
    Check(runtime.heads[0x2000u] == 0x3040u &&
              runtime.nodes[0x3040u].next == 0x3020u &&
              runtime.nodes[0x3020u].previous == 0x3040u &&
              runtime.nodes[0x3020u].next == 0x3000u &&
              first.previous == 0x3020u,
          "head insertion links previous and next neighbors");
    Check(pal4::SpatialGrid::RemoveNode(
              0x3020u, runtime) == 0x3020u &&
              runtime.nodes[0x3040u].next == 0x3000u &&
              first.previous == 0x3040u &&
              runtime.nodes[0x3020u].head_slot == 0u,
          "middle removal splices both neighbors");
    (void)pal4::SpatialGrid::RemoveNode(0x3040u, runtime);
    Check(runtime.heads[0x2000u] == 0x3000u &&
              first.previous == 0u,
          "head removal transfers bucket head");
    (void)pal4::SpatialGrid::RemoveNode(0x3000u, runtime);
    Check(runtime.heads[0x2000u] == 0u &&
              first.previous == 0u && first.next == 0u &&
              first.head_slot == 0u && first.payload == 0x77u,
          "sole-node removal clears links and retains payload");
    (void)pal4::SpatialGrid::RemoveNode(0x3000u, runtime);
    Check(first.head_slot == 0u && runtime.heads[0x2000u] == 0u,
          "repeat removal of detached node is inert");
}

void MoveAcrossBuckets() {
    Runtime runtime;
    runtime.grid.origin_bits = {
        std::bit_cast<std::uint32_t>(0.0f),
        std::bit_cast<std::uint32_t>(0.0f),
        std::bit_cast<std::uint32_t>(0.0f)};
    runtime.grid.extent_bits = {
        std::bit_cast<std::uint32_t>(10.0f),
        std::bit_cast<std::uint32_t>(10.0f),
        std::bit_cast<std::uint32_t>(10.0f)};
    runtime.grid.dimensions = {2u, 2u, 2u};
    runtime.grid.buckets = 0x2000u;
    auto& node = runtime.nodes[0x3000u];
    (void)pal4::SpatialGrid::InitializeNode(node, 0x99u);
    Check(pal4::SpatialGrid::UpdateNode(
              0x1000u, 0x3000u, 1.0f, 2.0f, 3.0f,
              runtime) == 0x2000u &&
              node.head_slot == 0x2000u &&
              runtime.heads[0x2000u] == 0x3000u &&
              node.position_bits ==
                  std::array<std::uint32_t, 3u>{
                      0x3f800000u, 0x40000000u, 0x40400000u},
          "first position links node into selected bucket");
    Check(pal4::SpatialGrid::UpdateNode(
              0x1000u, 0x3000u, 2.0f, 3.0f, 4.0f,
              runtime) == 0x2000u &&
              node.head_slot == 0x2000u &&
              runtime.heads[0x2000u] == 0x3000u,
          "same bucket updates position without relinking");
    Check(pal4::SpatialGrid::UpdateNode(
              0x1000u, 0x3000u, 7.0f, 2.0f, 3.0f,
              runtime) == 0x2010u &&
              runtime.heads[0x2000u] == 0u &&
              runtime.heads[0x2010u] == 0x3000u &&
              node.head_slot == 0x2010u,
          "cross-cell position unlinks old bucket then inserts new");
    Check(pal4::SpatialGrid::UpdateNode(
              0x1000u, 0x3000u, 20.0f, 2.0f, 3.0f,
              runtime) == 0x1028u &&
              runtime.heads[0x2010u] == 0u &&
              runtime.heads[0x1028u] == 0x3000u &&
              node.head_slot == 0x1028u,
          "out-of-range position moves node to overflow chain");
}

}  // namespace

int main() {
    InitializeAndList();
    MoveAcrossBuckets();
    std::cout << "Spatial grid node tests passed\n";
}
