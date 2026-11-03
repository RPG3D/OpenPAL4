#include "pal4/spatial_grid_wrapper.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

namespace {

void Check(const bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

class Runtime final : public pal4::ISpatialGridNodeRuntime {
public:
    std::uint32_t Allocate(std::uint32_t bytes) override {
        allocation_size = bytes;
        return allocation_result;
    }
    std::uint32_t Free(const std::uint32_t address) override {
        frees.push_back(address);
        return address + 1u;
    }
    pal4::SpatialGridState& GridAt(const std::uint32_t address) override {
        Check(address == 0x1000u, "known grid address");
        return grid;
    }
    pal4::SpatialGridNodeState& NodeAt(
        const std::uint32_t address) override {
        Check(nodes.contains(address), "known node address");
        return nodes.at(address);
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        return heads[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        heads[address] = value;
    }
    std::uint32_t allocation_size = 0u;
    std::uint32_t allocation_result = 0u;
    pal4::SpatialGridState grid{};
    std::map<std::uint32_t, pal4::SpatialGridNodeState> nodes;
    std::map<std::uint32_t, std::uint32_t> heads;
    std::vector<std::uint32_t> frees;
};

void AssociationFactory() {
    Runtime runtime;
    pal4::SpatialGridWrapperState wrapper{0x84b6b4u, 0x1000u};
    Check(pal4::SpatialGridWrapper::CreateNode(wrapper, 0x7000u, runtime) == 0u &&
              runtime.allocation_size == 36u && runtime.heads.empty(),
          "association allocation failure does not touch storage");
    runtime.allocation_result = 0x4000u;
    auto& node = runtime.nodes[0x4004u];
    node = {0xaaaaaaaa, 0xbbbbbbbb, 0xcccccccc, 0xdddddddd,
            {0x11223344u, 0x55667788u, 0x99aabbccu}};
    Check(pal4::SpatialGridWrapper::CreateNode(wrapper, 0x7000u, runtime) == 0x4000u &&
              runtime.heads[0x4000u] == 0x84b6c0u &&
              runtime.heads[0x4020u] == 0x1000u && node.payload == 0x7000u &&
              node.previous == 0u && node.next == 0u && node.head_slot == 0u &&
              node.position_bits == std::array<std::uint32_t, 3u>{
                  0x11223344u, 0x55667788u, 0x99aabbccu},
          "association factory matches native layout and preserves untouched position bytes");
}

void CountAndDestroy() {
    Runtime runtime;
    runtime.grid.dimensions = {1u, 1u, 2u};
    runtime.grid.buckets = 0x2000u;
    runtime.grid.overflow_head = 0x3040u;
    runtime.heads[0x2000u] = 0x3000u;
    runtime.heads[0x2004u] = 0x3020u;
    runtime.nodes[0x3000u].next = 0x3010u;
    runtime.nodes[0x3010u].payload = 8u;
    runtime.nodes[0x3020u].payload = 9u;
    runtime.nodes[0x3040u].payload = 10u;
    pal4::SpatialGridWrapperState wrapper{0x84b6b4u, 0x1000u};
    Check(pal4::SpatialGridWrapper::CountNodes(wrapper, runtime) == 4u,
          "wrapper counts all bucket and overflow nodes");
    std::uint32_t count = 0xffffffffu;
    Check(pal4::SpatialGridWrapper::IncrementCount(count, 0u, 0u) == 0u &&
              count == 0u,
          "callback increments 32-bit count with native wraparound");
    Check(pal4::SpatialGridWrapper::DestroyBase(wrapper, runtime) ==
              0x1001u &&
              runtime.frees ==
                  std::vector<std::uint32_t>{0x2000u, 0x1000u} &&
              wrapper.grid == 0u && wrapper.vtable == 0x73bbf3u,
          "base destructor frees bucket array then grid header");
    Check(pal4::SpatialGridWrapper::CountNodes(wrapper, runtime) == 0u,
          "empty wrapper count is zero");
}

void DeletingDestructors() {
    Runtime runtime;
    runtime.grid.buckets = 0x2000u;
    pal4::SpatialGridWrapperState wrapper{0u, 0x1000u};
    Check(pal4::SpatialGridWrapper::Destroy(
              0x4000u, wrapper, true, runtime) == 0x4000u &&
              runtime.frees ==
                  std::vector<std::uint32_t>{0x2000u, 0x1000u,
                                             0x4000u},
          "deleting destructor frees nested grid then wrapper");
    runtime.frees.clear();
    Check(pal4::SpatialGridWrapper::DestroyFacet(
              0x4000u, wrapper, false, runtime) == 0x4000u &&
              runtime.frees.empty() && wrapper.vtable == 0x73bbf3u,
          "facet destructor only installs its vtable when not deleting");
    (void)pal4::SpatialGridWrapper::DestroyFacet(
        0x4000u, wrapper, true, runtime);
    Check(runtime.frees == std::vector<std::uint32_t>{0x4000u},
          "deleting facet destructor frees the wrapper itself");
}

}  // namespace

int main() {
    AssociationFactory();
    CountAndDestroy();
    DeletingDestructors();
    std::cout << "Spatial grid wrapper tests passed\n";
}
