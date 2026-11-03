#include "pal4/spatial_grid.h"

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Visit final {
    std::uint32_t payload;
    std::uint32_t distance_bits;
    std::uint32_t context;
};
std::vector<Visit> visits;

std::uint32_t Record(const std::uint32_t payload,
                     const std::uint32_t distance_bits,
                     const std::uint32_t context) {
    visits.push_back({payload, distance_bits, context});
    return payload + 100u;
}

class Runtime final : public pal4::ISpatialGridNodeRuntime {
public:
    std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    std::uint32_t Free(std::uint32_t) override { return 0u; }
    pal4::SpatialGridState& GridAt(std::uint32_t address) override {
        Check(address == 0x1000u, "grid address");
        return grid;
    }
    pal4::SpatialGridNodeState& NodeAt(std::uint32_t address) override {
        Check(nodes.contains(address), "known node address");
        return nodes.at(address);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return heads[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        heads[address] = value;
    }
    pal4::SpatialGridState grid{};
    std::map<std::uint32_t, pal4::SpatialGridNodeState> nodes;
    std::map<std::uint32_t, std::uint32_t> heads;
};

void Add(Runtime& runtime, const std::uint32_t address,
         const std::uint32_t payload, const float x, const float y,
         const float z, const std::uint32_t next = 0u) {
    runtime.nodes[address] = {0u, next, 0u, payload,
                              {std::bit_cast<std::uint32_t>(x),
                               std::bit_cast<std::uint32_t>(y),
                               std::bit_cast<std::uint32_t>(z)}};
}

Runtime MakeRuntime() {
    Runtime runtime;
    runtime.grid.origin_bits = {0u, 0u, 0u};
    runtime.grid.extent_bits = {std::bit_cast<std::uint32_t>(10.0f),
                                std::bit_cast<std::uint32_t>(10.0f),
                                std::bit_cast<std::uint32_t>(10.0f)};
    runtime.grid.dimensions = {2u, 2u, 2u};
    runtime.grid.buckets = 0x2000u;
    Add(runtime, 0x3000u, 1u, 1.0f, 1.0f, 1.0f, 0x3020u);
    Add(runtime, 0x3020u, 2u, 4.0f, 1.0f, 1.0f);
    Add(runtime, 0x3040u, 3u, 6.0f, 1.0f, 1.0f);
    Add(runtime, 0x3060u, 4u, -1.0f, 1.0f, 1.0f);
    runtime.heads[0x2000u] = 0x3000u;
    runtime.heads[0x2010u] = 0x3040u;
    runtime.grid.overflow_head = 0x3060u;
    return runtime;
}

void CellAndOverflowQueries() {
    auto runtime = MakeRuntime();
    visits.clear();
    Check(pal4::SpatialGrid::ProcessSphereCells(
              0x1000u, 1.0f, 1.0f, 1.0f, 3.0f, Record, 77u,
              {0, 0, 0}, {1, 0, 0}, runtime) == 0u &&
              visits.size() == 1u && visits[0].payload == 1u &&
              visits[0].distance_bits == 0u && visits[0].context == 77u,
          "cell query uses strict radius and forwards float bits");
    Check(pal4::SpatialGrid::ProcessSphereCells(
              0x1000u, 1.0f, 1.0f, 1.0f, 3.0f, Record, 0u,
              {1, 0, 0}, {0, 0, 0}, runtime) == 0u,
          "reversed X span returns the last X index");
    visits.clear();
    Check(pal4::SpatialGrid::ProcessSphereOverflow(
              0x1000u, 0.0f, 1.0f, 1.0f, 2.0f, Record, 8u,
              runtime) == 104u && visits.size() == 1u &&
              visits[0].payload == 4u &&
              std::bit_cast<float>(visits[0].distance_bits) == 1.0f,
          "overflow visits matching nodes and returns callback result");
}

void DispatchAndTraversal() {
    auto runtime = MakeRuntime();
    visits.clear();
    Check(pal4::SpatialGrid::ProcessSphere(
              0x1000u, 1.0f, 1.0f, 1.0f, 0.5f, Record, 5u,
              runtime) == 0u && visits.size() == 1u &&
              visits[0].payload == 1u,
          "contained sphere scans only selected cells");
    visits.clear();
    (void)pal4::SpatialGrid::ProcessSphere(
        0x1000u, 0.0f, 1.0f, 1.0f, 5.1f, Record, 5u, runtime);
    Check(visits.size() == 3u && visits[0].payload == 4u &&
              visits[1].payload == 1u && visits[2].payload == 2u,
          "boundary crossing visits overflow before grid cells");
    visits.clear();
    Check(pal4::SpatialGrid::TraverseNodes(
              0x3000u, Record, 9u, runtime) == 102u &&
              visits.size() == 2u && visits[0].distance_bits == 0u &&
              visits[1].payload == 2u,
          "chain traversal returns final callback result");
    visits.clear();
    Check(pal4::SpatialGrid::ProcessAllNodes(
              0x1000u, Record, 3u, runtime) == 104u &&
              visits.size() == 4u && visits[0].payload == 1u &&
              visits[1].payload == 2u && visits[2].payload == 3u &&
              visits[3].payload == 4u,
          "all-nodes traversal visits buckets then overflow");
}

}  // namespace

int main() {
    CellAndOverflowQueries();
    DispatchAndTraversal();
    std::cout << "Spatial grid query tests passed\n";
}
