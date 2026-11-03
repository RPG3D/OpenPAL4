#include "pal4/native_behavior_path_endpoint_configuration.h"
#include "pal4/native_behavior_grid_association_runtime.h"
#include "pal4/native_behavior_path_endpoint_storage.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_monster_manager_owner.h"
#include "pal4/native_world_path_planner_owner.h"
#include "pal4/pathway_grid.h"
#include "pal4/grid_node_object.h"
#include "pal4/spatial_grid_wrapper.h"

#include <array>
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace {
void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
template <class T> T& At(std::uint32_t address) {
    return *reinterpret_cast<T*>(address);
}
using NativeVectors = pal4::NativeBehaviorPathEndpointConfiguration;
}  // namespace

int main(int argc, char** argv) {
    try {
        pal4::NativeMonsterManagerOwner manager;
        (void)manager.Construct();
        Check(manager.InitializeScene({}), "initialize real monster manager spatial grid");
        const auto wrapper_address = At<std::uint32_t>(manager.Address() + 0x58u);
        auto& wrapper = At<pal4::SpatialGridWrapperState>(wrapper_address);
        pal4::NativeWorldPathPlannerOwner paths;
        (void)paths.Construct();
        std::uint32_t path = 0u;
        if (argc == 2) {
            Check(paths.LoadScene(argv[1], "Q02", "Q02"), "load original five-path scene");
            path = paths.FindPath("4064");
            Check(path != 0u, "resolve original 17-point path 4064");
        }
        const auto stable_paths = paths.OutstandingAllocations();
        const auto stable_manager = manager.OutstandingAllocations();
        pal4::NativeBaseObjectMemory memory;
        pal4::NativeBehaviorGridAssociationRuntime associations(memory);
        std::uint32_t identifier = 0u;
        pal4::NativeBehaviorPathEndpointStorage storage(memory, identifier,
            [&](std::uint32_t address, bool free_self) {
                associations.DestroyAssociation(address, free_self);
            });
        const std::array<std::uint32_t, 7> samples{0u, 0u, 0u, 16384u, 24575u, 16384u, 0u};
        std::size_t draws = 0u;
        NativeVectors configuration(memory, associations, [&] {
            const auto value = (draws % samples.size() == 6u &&
                (draws / samples.size()) % 2u != 0u) ? 32767u : samples[draws % samples.size()];
            ++draws;
            return value;
        });
        for (auto cycle = 0u; cycle < 3u; ++cycle) {
            const auto address = memory.Allocate(sizeof(pal4::BehaviorPathEndpointState));
            storage.Construct(address);
            auto& helper = At<pal4::BehaviorPathEndpointState>(address);
            helper.words[13] = 0xdeadbeefu;
            helper.words[14] = 0xbeefdeadu;
            const auto result = configuration.Configure(address, wrapper_address, path);
            Check(draws == (cycle * 2u + 1u) * samples.size(),
                  "activation reuses shared rejection sampler with original rand order");
            Check(result == pal4::SpatialGrid::BucketAddress(
                      associations.GridAt(wrapper.grid), wrapper.grid, 0.0f, 0.0f, 0.0f) &&
                      helper.words[47] == path && helper.words[48] == 1u &&
                      helper.words[13] == 0u && helper.words[14] == 0u &&
                      (helper.words[21] & 0xffu) == 0u,
                  "real initialization resets movement and notifies spatial association");
            Check(helper.words[23] == 0x3f800000u && helper.words[24] == 0x3f000000u &&
                      helper.words[25] == 0u && helper.words[26] == 0x3dcccccdu &&
                      helper.words[27] == 0x3f800000u,
                  "activation calls existing five scalar setters with original defaults");
            for (const auto first : {29u, 32u, 35u, 39u})
                for (auto axis = 0u; axis < 3u; ++axis)
                    Check(helper.words[first + axis] == 0u, "activation resets four motion vectors");
            const auto direction = NativeVectors::ReadVector(helper, 0x10u);
            const auto first_axis = NativeVectors::ReadVector(helper, 0x00u);
            Check(std::fabs(direction[0] - 1.0f) < 0.001f && direction[1] == 0.0f &&
                      std::fabs(direction[2]) < 0.001f &&
                      first_axis == pal4::BehaviorVector3{direction[2], direction[1], direction[0]} &&
                      NativeVectors::ReadVector(helper, 0x08u) == pal4::BehaviorVector3{0.0f, 1.0f, 0.0f} &&
                      NativeVectors::ReadVector(helper, 0x18u) == pal4::BehaviorVector3{},
                  "real initialization writes normalized random direction and reordered first axis");
            const auto node_address = helper.words[42];
            const auto& node = At<pal4::GridNodeObjectState>(node_address);
            Check(node.node.payload == address && node.node.head_slot == result &&
                      node.node.position_bits == std::array<std::uint32_t, 3>{} &&
                      pal4::SpatialGridWrapper::CountNodes(wrapper, associations) == 1u,
                  "activation inserts actual helper into actual spatial grid at initialized position");
            if (path) {
                const auto& grid = At<pal4::PathwayGridState>(path);
                Check(std::memcmp(helper.words.data() + 50u,
                                  reinterpret_cast<void*>(grid.positions), 12u) == 0 &&
                          std::memcmp(helper.words.data() + 53u,
                            reinterpret_cast<void*>(grid.positions + (grid.point_count - 1u) * 12u), 12u) == 0,
                      "configuration caches endpoints from original path 4064");
            }
            const auto endpoints = std::array<std::uint32_t, 6>{
                helper.words[50], helper.words[51], helper.words[52],
                helper.words[53], helper.words[54], helper.words[55]};
            const auto stable = memory.OutstandingAllocations();
            (void)configuration.Configure(address, wrapper_address, 0u);
            Check(draws == (cycle * 2u + 2u) * samples.size() && helper.words[47] == 0u && helper.words[48] == 0xffffffffu &&
                      memory.OutstandingAllocations() == stable &&
                      std::memcmp(endpoints.data(), helper.words.data() + 50u, 24u) == 0 &&
                      pal4::SpatialGridWrapper::CountNodes(wrapper, associations) == 1u,
                  "null-path reconfiguration rebinds and initializes while preserving cached endpoints");
            NativeVectors::SetVector(helper, 0x1cu, {2.0f, 3.0f, 4.0f});
            NativeVectors::SetVector(helper, 0x04u, {4.0f, 5.0f, 6.0f});
            NativeVectors::SetVector(helper, 0x0cu, {0.0f, 2.0f, 0.0f});
            NativeVectors::SetVector(helper, 0x14u, {6.0f, 5.0f, 4.0f});
            Check(NativeVectors::ReadVector(helper, 0x18u) == pal4::BehaviorVector3{2.0f, 3.0f, 4.0f} &&
                      NativeVectors::ReadVector(helper, 0x00u) == pal4::BehaviorVector3{4.0f, 5.0f, 6.0f} &&
                      NativeVectors::ReadVector(helper, 0x08u) == pal4::BehaviorVector3{0.0f, 2.0f, 0.0f} &&
                      NativeVectors::ReadVector(helper, 0x10u) == pal4::BehaviorVector3{6.0f, 5.0f, 4.0f},
                  "C03 vector slots preserve separate basis and position rows");
            storage.Destruct(address);
            (void)memory.Free(address);
            Check(memory.OutstandingAllocations() == 0u &&
                      pal4::SpatialGridWrapper::CountNodes(wrapper, associations) == 0u &&
                      manager.OutstandingAllocations() == stable_manager &&
                      paths.OutstandingAllocations() == stable_paths,
                  "activation cycles release helper/association and preserve borrowed path/grid owners");
        }
        // Native C++ random services can throw. Discard must reclaim the
        // association even if activation failed before its first grid insert.
        const auto failed = memory.Allocate(sizeof(pal4::BehaviorPathEndpointState));
        storage.Construct(failed);
        NativeVectors failing(memory, associations, []() -> std::uint32_t {
            throw std::runtime_error("injected shared random failure");
        });
        bool caught = false;
        try { (void)failing.Configure(failed, wrapper_address, path); }
        catch (const std::runtime_error&) { caught = true; }
        Check(caught && At<pal4::BehaviorPathEndpointState>(failed).words[42] != 0u,
              "failed activation leaves association owned by helper for discard");
        storage.Destruct(failed);
        (void)memory.Free(failed);
        Check(memory.OutstandingAllocations() == 0u &&
                  pal4::SpatialGridWrapper::CountNodes(wrapper, associations) == 0u,
              "failed activation discards without stale bucket or allocation");
        paths.Destroy();
        manager.Destroy();
        Check(paths.OutstandingAllocations() == 0u && manager.OutstandingAllocations() == 0u,
              "borrowed owners finish independent cleanup");
        std::puts("PASS: native path endpoints, movement initialization, shared random sampling and spatial activation");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        return 1;
    }
}
