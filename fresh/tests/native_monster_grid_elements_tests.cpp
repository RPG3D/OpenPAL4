#include "pal4/native_monster_manager_owner.h"
#include "pal4/native_behavior_path_endpoint_configuration.h"
#include "pal4/native_behavior_path_endpoint_storage.h"
#include "pal4/native_behavior_grid_association_runtime.h"
#include "pal4/grid_manager_members.h"
#include "pal4/monster_behavior_manager.h"
#include "pal4/spatial_grid_wrapper.h"

#include <array>
#include <bit>
#include <limits>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
template <class T> T& At(std::uint32_t address) {
    return *reinterpret_cast<T*>(static_cast<std::uintptr_t>(address));
}

// Actual helper construction/association/destruction, with explicitly supplied
// test updates. This does not claim the helper's original physics is bound.
class Helpers final : public pal4::INativeMonsterGridElementRuntime {
public:
    pal4::NativeBaseObjectMemory memory;
    pal4::NativeBehaviorGridAssociationRuntime associations{memory};
    std::uint32_t identifier = 0;
    pal4::NativeBehaviorPathEndpointStorage storage{memory, identifier,
        [this](std::uint32_t node, bool free_self) {
            associations.DestroyAssociation(node, free_self);
        }};
    std::vector<std::uint32_t> updates;
    std::array<std::uint32_t, 16> released{};
    unsigned release_count = 0;
    bool fail_update = false;
    pal4::NativeMonsterManagerOwner* manager = nullptr;

    std::uint32_t Create(std::uint32_t wrapper) {
        const auto element = memory.Allocate(sizeof(pal4::BehaviorPathEndpointState));
        storage.Construct(element);
        unsigned draws = 0;
        pal4::NativeBehaviorPathEndpointConfiguration configuration(memory, associations,
            [&] { constexpr std::array<std::uint32_t, 4> samples{16384, 24575, 16384, 0};
                return samples[draws++ % samples.size()]; });
        (void)configuration.Configure(element, wrapper, 0);
        return element;
    }
    void Release(std::uint32_t element) noexcept override {
        released[release_count++] = element;
        storage.Destruct(element);
        (void)memory.Free(element);
    }
    void Update(std::uint32_t element, pal4::GridManagerMembersState& members,
                 std::uint32_t argument) override {
        Check(manager && &members == &At<pal4::GridManagerMembersState>(
            manager->Address() + 0x38u) && argument == 0x3f000000u,
            "native grid update preserves member context and argument bits");
        updates.push_back(element);
        if (fail_update) {
            bool remove_rejected = false, reload_rejected = false;
            try { (void)manager->RemoveGridElement(element); }
            catch (const std::logic_error&) { remove_rejected = true; }
            try { (void)manager->InitializeScene({}); }
            catch (const std::logic_error&) { reload_rejected = true; }
            Check(remove_rejected && reload_rejected,
                "update cannot invalidate its collection or borrowed grid");
            throw std::runtime_error("test update failure");
        }
    }
};
}

int main() try {
    pal4::NativeMonsterManagerOwner manager;
    (void)manager.Construct();
    auto helpers = std::make_shared<Helpers>();
    helpers->manager = &manager;
    // Without a grid registration fails and lifetime stays with the caller.
    const auto unaccepted = helpers->memory.Allocate(4);
    Check(!manager.AddGridElement(unaccepted, helpers) && manager.GridElementCount() == 0,
        "no wrapper means no ownership transfer");
    (void)helpers->memory.Free(unaccepted);
    Check(manager.InitializeScene({}), "initialize manager spatial grid");
    std::array<pal4::MonsterBehaviorManagerState, 3> actors{};
    const std::array<const char*, 3> names{"z", "a", "m"};
    for (unsigned i = 0; i < actors.size(); ++i) {
        actors[i].words[0x1f0u / 4u] = i == 2u ? 0u : 1u;
        Check(manager.RegisterOwnedObject(names[i],
            reinterpret_cast<std::uint32_t>(&actors[i]), std::make_unique<int>(0)),
            "register world update image lifetime");
    }
    // Snare changes the per-object branch, not the manager's active filter.
    actors[1].words[0x730u / 4u] = 1u;
    const auto first_actor = reinterpret_cast<std::uint32_t>(&actors[1]);
    const auto last_actor = reinterpret_cast<std::uint32_t>(&actors[0]);
    std::size_t stable_manager = 0;
    for (unsigned cycle = 0; cycle < 3; ++cycle) {
        auto& members = At<pal4::GridManagerMembersState>(manager.Address() + 0x38u);
        auto& wrapper = At<pal4::SpatialGridWrapperState>(members.grid_wrapper);
        std::array<std::uint32_t, 3> elements{};
        for (auto& element : elements) {
            element = helpers->Create(members.grid_wrapper);
            Check(manager.AddGridElement(element, helpers), "register real helper");
        }
        Check(manager.GridElementCount() == 3 &&
            (members.elements_capacity - members.elements_begin) / 4u == 4u &&
            pal4::SpatialGridWrapper::CountNodes(wrapper, helpers->associations) == 3,
            "manager-owned collection grows 1/2/4 while helpers own associations");
        bool duplicate_rejected = false;
        try { (void)manager.AddGridElement(elements[0], helpers); }
        catch (const std::logic_error&) { duplicate_rejected = true; }
        Check(duplicate_rejected && manager.GridElementCount() == 3,
            "duplicate cannot create double release");
        const auto releases = helpers->release_count;
        Check(manager.RemoveGridElement(elements[1]) && manager.GridElementCount() == 2 &&
            At<std::uint32_t>(members.elements_begin + 4) == elements[2] &&
            helpers->release_count == releases + 1 &&
            helpers->released[releases] == elements[1] &&
            pal4::SpatialGridWrapper::CountNodes(wrapper, helpers->associations) == 2,
            "middle removal compacts collection and destroys helper/association exactly once");
        Check(!manager.RemoveGridElement(elements[1]), "missing removal does not release twice");
        helpers->updates.clear();
        std::vector<std::uint32_t> actors_updated;
        bool fail_world = true;
        auto dispatch = [&](std::uint32_t object, float elapsed, std::uint32_t mode) {
            Check(std::bit_cast<std::uint32_t>(elapsed) == 0x3f000000u && mode == 19u,
                "world dispatch preserves float bits and second virtual argument");
            Check(helpers->updates == std::vector<std::uint32_t>{elements[0], elements[2]},
                "extended world updates grid before any active actor");
            actors_updated.push_back(object);
            // Pursuit changes and group traversal must remain available to AI.
            (void)manager.SetPursuitTargetAll(0x12340000u);
            unsigned group_visits{};
            (void)manager.ForEachObject([&](auto value) { ++group_visits; return value; });
            Check(group_visits == 3u, "world callback can visit stable monster group");
            if (!fail_world) return;
            for (unsigned operation = 0; operation < 7u; ++operation) {
                bool rejected{};
                try {
                    if (operation == 0u) manager.Destroy();
                    if (operation == 1u) (void)manager.InitializeScene({});
                    if (operation == 2u) (void)manager.LoadInfo({});
                    if (operation == 3u) (void)manager.RegisterConstructedObject("extra", object);
                    if (operation == 4u) manager.CleanupScene([](auto) {});
                    if (operation == 5u) (void)manager.ProcessWorldObjects(0.5f, 19u, [](auto, auto, auto) {});
                    if (operation == 6u) (void)manager.RenderFrame(1u);
                } catch (const std::logic_error&) { rejected = true; }
                Check(rejected && manager.ObjectCount() == 3u,
                    "world update pins creating collection and rejects recursive phases");
            }
            throw std::runtime_error("intentional world update failure");
        };
        bool world_failed{};
        try { (void)manager.ProcessWorldObjects(0.5f, 19u, dispatch); }
        catch (const std::runtime_error& error) {
            world_failed = std::string_view(error.what()) == "intentional world update failure";
        }
        Check(world_failed && actors_updated == std::vector<std::uint32_t>{first_actor},
            "world failure stops ordered active traversal without destroying owners");
        helpers->updates.clear(); actors_updated.clear(); fail_world = false;
        (void)manager.ProcessWorldObjects(0.5f, 19u, dispatch);
        Check(actors_updated == std::vector<std::uint32_t>{first_actor, last_actor},
            "world retry restores guard, includes snared active actor and skips inactive actor");
        auto* grid_enabled = reinterpret_cast<std::uint8_t*>(manager.Address() + 0x5cu);
        const auto saved_grid_enabled = *grid_enabled;
        *grid_enabled = 0u;
        helpers->updates.clear(); actors_updated.clear();
        (void)manager.ProcessWorldObjects(0.5f, 19u,
            [&](auto object, auto, auto) { actors_updated.push_back(object); });
        Check(helpers->updates.empty() &&
              actors_updated == std::vector<std::uint32_t>{first_actor, last_actor},
            "disabled manager grid still updates active world actors");
        *grid_enabled = saved_grid_enabled;
        for (auto invalid : {-0.5f, std::numeric_limits<float>::infinity(),
                              std::numeric_limits<float>::quiet_NaN()}) {
            bool rejected{};
            try { (void)manager.ProcessWorldObjects(invalid, 19u, dispatch); }
            catch (const std::invalid_argument&) { rejected = true; }
            Check(rejected, "invalid world delta rejects before grid and actor dispatch");
        }
        helpers->updates.clear();
        (void)manager.UpdateGridElements(0x3f000000u);
        Check(helpers->updates == std::vector<std::uint32_t>{elements[0], elements[2]},
            "original update iteration preserves compacted order");
        members.update_disabled = 7;
        helpers->updates.clear();
        Check(manager.UpdateGridElements(0x3f000000u) == 7 && helpers->updates.empty(),
            "disabled update returns original nonzero byte without dispatch");
        members.update_disabled = 0;
        helpers->fail_update = true;
        bool failed = false;
        try { (void)manager.UpdateGridElements(0x3f000000u); }
        catch (const std::runtime_error&) { failed = true; }
        Check(failed && manager.GridElementCount() == 2, "update failure retains collection");
        helpers->fail_update = false;
        helpers->updates.clear();
        (void)manager.UpdateGridElements(0x3f000000u);
        Check(helpers->updates.size() == 2, "update failure clears reentry guard for retry");
        Check(manager.InitializeScene({}) && manager.GridElementCount() == 0 &&
            helpers->release_count == releases + 3 &&
            helpers->released[releases + 1] == elements[0] &&
            helpers->released[releases + 2] == elements[2] &&
            helpers->memory.OutstandingAllocations() == 0,
            "scene reload releases helpers in order before destroying borrowed grid");
        // Scene reload clears the tree; restore the borrowed test images.
        for (unsigned i = 0; i < actors.size(); ++i)
            Check(manager.RegisterOwnedObject(names[i],
                reinterpret_cast<std::uint32_t>(&actors[i]), std::make_unique<int>(0)),
                "restore world test image lifetimes after reload");
        if (!cycle) stable_manager = manager.OutstandingAllocations();
        else Check(manager.OutstandingAllocations() == stable_manager,
            "repeated reload retains manager buffers without leaking");
    }
    auto& members = At<pal4::GridManagerMembersState>(manager.Address() + 0x38u);
    const auto last = helpers->Create(members.grid_wrapper);
    Check(manager.AddGridElement(last, helpers), "register helper for retained-service teardown");
    std::weak_ptr<Helpers> retained = helpers;
    helpers.reset();
    Check(!retained.expired(), "manager retains dispatch after caller drops its facade");
    manager.Destroy();
    Check(retained.expired() && manager.OutstandingAllocations() == 0 &&
        manager.GridElementCount() == 0,
        "manager destruction releases helper/service before grid and all native allocations");
    std::puts("PASS: native monster grid helper ownership/reload/update dispatch");
    return 0;
} catch (const std::exception& error) {
    std::fprintf(stderr, "FAIL: %s\n", error.what());
    return 1;
}
