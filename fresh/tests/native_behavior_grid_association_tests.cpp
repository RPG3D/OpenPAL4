#include "pal4/native_behavior_grid_association_runtime.h"
#include "pal4/native_behavior_path_endpoint_storage.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_monster_manager_owner.h"
#include "pal4/grid_node_object.h"
#include "pal4/spatial_grid_wrapper.h"

#include <bit>
#include <cstdio>
#include <stdexcept>

namespace {
void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
template <class T> T& At(std::uint32_t address) {
    return *reinterpret_cast<T*>(address);
}
}  // namespace

int main() {
    try {
        pal4::NativeMonsterManagerOwner manager;
        (void)manager.Construct();
        Check(manager.InitializeScene({}), "initialize real monster manager grid");
        const auto wrapper_address = At<std::uint32_t>(manager.Address() + 0x58u);
        auto& wrapper = At<pal4::SpatialGridWrapperState>(wrapper_address);
        Check(wrapper.grid == manager.GridAddress(), "borrow actual manager wrapper");
        const auto manager_allocations = manager.OutstandingAllocations();
        pal4::NativeBaseObjectMemory memory;
        pal4::NativeBehaviorGridAssociationRuntime associations(memory);
        std::uint32_t identifier = 0u;
        pal4::NativeBehaviorPathEndpointStorage storage(memory, identifier,
            [&](std::uint32_t address, bool free_self) {
                associations.DestroyAssociation(address, free_self);
            });
        for (auto cycle = 0u; cycle < 3u; ++cycle) {
            const auto first_address = memory.Allocate(sizeof(pal4::BehaviorPathEndpointState));
            const auto second_address = memory.Allocate(sizeof(pal4::BehaviorPathEndpointState));
            storage.Construct(first_address);
            storage.Construct(second_address);
            auto& first = At<pal4::BehaviorPathEndpointState>(first_address);
            auto& second = At<pal4::BehaviorPathEndpointState>(second_address);
            const auto first_node = associations.Bind(first, wrapper_address);
            const auto second_node = associations.Bind(second, wrapper_address);
            auto& a = At<pal4::GridNodeObjectState>(first_node);
            auto& b = At<pal4::GridNodeObjectState>(second_node);
            Check(a.vtable == 0x84b6c0u && a.grid == wrapper.grid &&
                      a.node.payload == first_address && !a.node.head_slot &&
                      first.words[42] == first_node,
                  "original factory image links helper and borrowed grid without inserting");
            (void)associations.UpdatePosition(first_node, {0.0f, 0.0f, 0.0f});
            (void)associations.UpdatePosition(second_node, {0.0f, 0.0f, 0.0f});
            const auto slot = a.node.head_slot;
            Check(b.node.next == first_node + 4u &&
                      a.node.previous == second_node + 4u &&
                      pal4::SpatialGridWrapper::CountNodes(wrapper, associations) == 2u,
                  "two real helper nodes share a consistent intrusive bucket");
            (void)associations.UpdatePosition(first_node, {100.0f, 0.0f, 0.0f});
            Check(a.node.head_slot == wrapper.grid + 40u && b.node.next == 0u &&
                      a.node.previous == 0u && a.node.position_bits[0] ==
                          std::bit_cast<std::uint32_t>(100.0f),
                  "moving outside original bounds unlinks bucket and enters overflow");
            (void)associations.UpdatePosition(first_node, {8.0f, 0.0f, 0.0f});
            Check(a.node.head_slot != slot &&
                      associations.GridAt(wrapper.grid).overflow_head == 0u,
                  "moving inside original bounds leaves overflow and selects new cell");
            std::uint32_t visits = 0u;
            (void)pal4::SpatialGrid::ProcessSphere(wrapper.grid, 8.0f, 0.0f, 0.0f, 1.0f,
                [&](std::uint32_t payload, std::uint32_t, std::uint32_t) {
                    Check(payload == first_address, "sphere query returns actual helper payload");
                    ++visits;
                    return 1u;
                }, 0u, associations);
            Check(visits == 1u, "existing native grid query discovers moved helper");
            const auto stable = memory.OutstandingAllocations();
            (void)associations.Bind(first, wrapper_address);
            Check(memory.OutstandingAllocations() == stable &&
                      pal4::SpatialGridWrapper::CountNodes(wrapper, associations) == 1u,
                  "rebinding destroys old inserted association without allocation growth");
            const auto rebound = first.words[42];
            (void)associations.UpdatePosition(rebound, {0.0f, 0.0f, 0.0f});
            storage.Destruct(second_address);
            Check(At<pal4::GridNodeObjectState>(rebound).node.next == 0u &&
                      At<pal4::GridNodeObjectState>(rebound).node.previous == 0u &&
                      pal4::SpatialGridWrapper::CountNodes(wrapper, associations) == 1u,
                  "helper cleanup removes a neighbor and repairs bucket links");
            storage.Destruct(first_address);
            (void)memory.Free(first_address);
            (void)memory.Free(second_address);
            Check(memory.OutstandingAllocations() == 0u &&
                      pal4::SpatialGridWrapper::CountNodes(wrapper, associations) == 0u &&
                      manager.OutstandingAllocations() == manager_allocations,
                  "helper cleanup releases owned associations and preserves borrowed manager grid");
        }
        Check(manager.InitializeScene({}), "grid resets after association teardown");
        manager.Destroy();
        Check(manager.OutstandingAllocations() == 0u, "manager completes independent cleanup");
        std::puts("PASS: native helper grid association, movement between cells, query, rebind and cleanup");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        return 1;
    }
}
