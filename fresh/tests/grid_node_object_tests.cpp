#include "pal4/grid_node_object.h"

#include <bit>
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

class GridRuntime final : public pal4::ISpatialGridNodeRuntime {
public:
    std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    std::uint32_t Free(const std::uint32_t address) override {
        freed.push_back(address);
        return address;
    }
    pal4::SpatialGridState& GridAt(const std::uint32_t address) override {
        Check(address == 0x1000u, "grid lookup address");
        return grid;
    }
    pal4::SpatialGridNodeState& NodeAt(
        const std::uint32_t address) override {
        if (address == 0x3004u)
            return object->node;
        Check(nodes.contains(address), "node lookup address");
        return nodes.at(address);
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        return heads[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        heads[address] = value;
    }
    pal4::GridNodeObjectState* object = nullptr;
    pal4::SpatialGridState grid{};
    std::map<std::uint32_t, pal4::SpatialGridNodeState> nodes;
    std::map<std::uint32_t, std::uint32_t> heads;
    std::vector<std::uint32_t> freed;
};

class CollectionRuntime final : public pal4::IGridManagerMembersRuntime {
public:
    std::uint32_t Allocate(const std::uint32_t bytes) override {
        allocation_sizes.push_back(bytes);
        const auto result = next_address;
        next_address += 0x100u;
        return result;
    }
    std::uint32_t Free(std::uint32_t) override { return 0u; }
    pal4::SpatialGridState& GridAt(std::uint32_t) override {
        return unused_grid;
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words.at(address);
    }
    void DestroyGridWrapper(std::uint32_t) override {}
    void WriteGridWrapper(std::uint32_t, std::uint32_t) override {}
    void ReleaseElement(std::uint32_t) override {}
    void UpdateElement(std::uint32_t,
                       pal4::GridManagerMembersState&,
                       std::uint32_t) override {}
    std::uint32_t next_address = 0x5000u;
    pal4::SpatialGridState unused_grid{};
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> allocation_sizes;
};

void UpdateCollectAndRemove() {
    GridRuntime grid_runtime;
    CollectionRuntime collection_runtime;
    pal4::GridNodeObjectState object{};
    grid_runtime.object = &object;
    object.grid = 0x1000u;
    object.node.payload = 0x44u;
    grid_runtime.grid.origin_bits = {0u, 0u, 0u};
    grid_runtime.grid.extent_bits = {
        std::bit_cast<std::uint32_t>(10.0f),
        std::bit_cast<std::uint32_t>(10.0f),
        std::bit_cast<std::uint32_t>(10.0f)};
    grid_runtime.grid.dimensions = {2u, 2u, 2u};
    grid_runtime.grid.buckets = 0x2000u;
    Check(pal4::GridNodeObject::UpdatePosition(
              0x3000u, object, 1.0f, 1.0f, 1.0f,
              grid_runtime) == 0x2000u &&
              grid_runtime.heads[0x2000u] == 0x3004u &&
              object.node.head_slot == 0x2000u,
          "node object delegates positioning using its +4 node address");
    auto& second = grid_runtime.nodes[0x4000u];
    second.payload = 0x55u;
    second.position_bits = {
        std::bit_cast<std::uint32_t>(2.0f),
        std::bit_cast<std::uint32_t>(1.0f),
        std::bit_cast<std::uint32_t>(1.0f)};
    (void)pal4::SpatialGrid::InsertNode(0x4000u, 0x2000u, grid_runtime);
    pal4::GridManagerMembersState collection{};
    Check(pal4::GridNodeObject::CollectSphere(
              object, 1.0f, 1.0f, 1.0f, 2.0f,
              collection, grid_runtime, collection_runtime) == 0u &&
              collection_runtime.allocation_sizes ==
                  std::vector<std::uint32_t>{4u, 8u} &&
              collection_runtime.words[collection.elements_begin] == 0x55u &&
              collection_runtime.words[collection.elements_begin + 4u] == 0x44u,
          "sphere wrapper appends matching payloads in list order");
    Check(pal4::GridNodeObject::RemoveFromGrid(
              0x3000u, object, grid_runtime) == 0x3004u &&
              object.vtable == 0x84b6d8u &&
              second.next == 0u && object.node.head_slot == 0u,
          "remove wrapper unlinks +4 node and installs base vtable");
    Check(pal4::GridNodeObject::DestroyFacet(
              0x3000u, object, false, grid_runtime) == 0x3000u &&
              grid_runtime.freed.empty(),
          "nondeleting facet destructor preserves allocation");
    (void)pal4::GridNodeObject::DestroyFacet(
        0x3000u, object, true, grid_runtime);
    Check(grid_runtime.freed == std::vector<std::uint32_t>{0x3000u},
          "deleting facet destructor frees object");
}

void VisitorAppendWithoutWrapper() {
    CollectionRuntime runtime;
    pal4::GridManagerMembersState collection{};
    Check(pal4::GridManagerMembers::AppendVisitedElement(
              collection, 0u, 0x3f800000u, runtime) == 0x5004u &&
              runtime.words[0x5000u] == 0u,
          "query callback appends payload even without wrapper or nonzero guard");
}

}  // namespace

int main() {
    UpdateCollectAndRemove();
    VisitorAppendWithoutWrapper();
    std::cout << "Grid node object tests passed\n";
}
