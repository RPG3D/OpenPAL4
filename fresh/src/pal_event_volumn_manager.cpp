#include "pal4/pal_event_volumn_manager.h"

#include <cstdint>
#include <bit>
#include <limits>

namespace pal4 {

std::uint32_t PalEventVolumnManager::RegisterSingleton(
    PalEventVolumnRegistrationState& state,
    IPalEventVolumnRegistryRuntime& runtime) {
    if ((state.guard & 1u) != 0u) return state.instance;
    state.guard = static_cast<std::uint8_t>(state.guard | 1u);
    const auto manager = runtime.SingletonManager();
    state.instance = runtime.Register(manager, "palEventVolumnMgr", 1012u,
                                      0x64ea50u, 0x64eab0u);
    return state.instance;
}

std::uint32_t PalEventVolumnManager::Create(
    PalEventVolumnSharedTree& shared, IPalEventVolumnMemory& memory) {
    const auto address = memory.Allocate(52u);
    if (address != 0u)
        (void)Initialize(memory.ObjectAt(address), shared, memory);
    return address;
}

PalEventVolumnState& PalEventVolumnManager::Initialize(
    PalEventVolumnState& state, PalEventVolumnSharedTree& shared,
    IPalEventVolumnMemory& memory) {
    state = {};
    const auto candidate = memory.Allocate(24u);
    memory.WriteWord(candidate, 4u, 0u);
    memory.WriteWord(candidate, 20u, 1u);
    if (shared.nil == 0u) {
        shared.nil = candidate;
        memory.WriteWord(candidate, 0u, 0u);
        memory.WriteWord(candidate, 8u, 0u);
    } else {
        memory.Free(candidate);
    }
    ++shared.references;

    const auto head = memory.Allocate(24u);
    memory.WriteWord(head, 4u, shared.nil);
    memory.WriteWord(head, 20u, 0u);
    memory.WriteWord(head, 0u, head);
    memory.WriteWord(head, 8u, head);
    state.words[10u] = head;
    state.words[12u] = 0u;
    state.words[8u] = 0x84b490u;
    state.words[5u] = 0u;
    state.words[6u] = 1u;
    state.words[7u] = 1u;
    state.words[4u] = 0u;
    return state;
}

int PalEventVolumnManager::Destroy(
    const std::uint32_t address, PalEventVolumnSharedTree& shared,
    IPalEventVolumnCleanupRuntime& runtime) {
    if (address == 0u) return 0;
    (void)Cleanup(runtime.ObjectAt(address), shared, runtime);
    return runtime.Free(address);
}

std::uint32_t PalEventVolumnManager::ClearRecords(
    PalEventVolumnState& state, IPalEventVolumnCleanupRuntime& runtime) {
    const auto end = state.words[2u];
    state.words[5u] = 0u;
    for (auto slot = state.words[1u]; slot != end; slot += 4u) {
        const auto record = runtime.ReadWord(slot);
        if (record != 0u) runtime.DestroyRecord(record);
    }
    state.words[2u] = state.words[1u];
    state.words[4u] = 0u;
    return end;
}

int PalEventVolumnManager::Cleanup(
    PalEventVolumnState& state, PalEventVolumnSharedTree& shared,
    IPalEventVolumnCleanupRuntime& runtime) {
    (void)ClearRecords(state, runtime);
    state.words[8u] = 0x84b490u;
    runtime.ClearTree(state, shared);
    (void)runtime.Free(state.words[10u]);
    state.words[10u] = 0u;
    state.words[12u] = 0u;
    if (--shared.references == 0u) {
        const auto nil = shared.nil;
        shared.nil = 0u;
        if (nil != 0u) (void)runtime.Free(nil);
    }
    const auto result = runtime.Free(state.words[1u]);
    state.words[1u] = 0u;
    state.words[2u] = 0u;
    state.words[3u] = 0u;
    return result;
}

std::uint32_t PalEventVolumnManager::PickAtPixel(
    PalEventVolumnState& state, const std::uint32_t camera,
    const std::uint32_t pixel, const bool include_inactive,
    IPalEventVolumnSelectionRuntime& runtime) {
    std::uint32_t nearest = 0u;
    auto nearest_distance = std::numeric_limits<float>::max();
    for (auto slot = state.words[1u]; slot != state.words[2u]; slot += 4u) {
        const auto address = runtime.ReadRecordPointer(slot);
        auto& record = runtime.RecordAt(address);
        (void)SceneEvfRecord::UpdateVertexColors(
            record, address, false, runtime.Colors());
        float distance = std::numeric_limits<float>::max();
        if ((include_inactive || record.words[19u] != 0u) &&
            SceneEvfRecord::IntersectsPixel(
                record, camera, pixel, &distance, runtime.Intersections())) {
            if (distance < nearest_distance) {
                nearest = address;
                nearest_distance = distance;
            }
            state.words[4u] = address;
        }
    }
    if (nearest != 0u) {
        (void)SceneEvfRecord::UpdateVertexColors(
            runtime.RecordAt(nearest), nearest, true, runtime.Colors());
        state.words[4u] = nearest;
    }
    return nearest;
}

std::uint32_t PalEventVolumnManager::FindByName(
    PalEventVolumnState& state, const std::uint32_t manager_address,
    const std::uint32_t temporary_address, const char* name,
    IPalEventVolumnNameRuntime& runtime) {
    SceneEvfRecordState query;
    (void)SceneEvfRecord::Initialize(query, temporary_address,
                                     manager_address, runtime);
    (void)SceneEvfRecord::SetName(query, name, runtime);
    std::uint32_t found = 0u;
    for (auto slot = state.words[1u]; slot != state.words[2u]; slot += 4u) {
        const auto record = runtime.ReadRecordPointer(slot);
        if (runtime.CompareRecordName(record, name)) {
            state.words[4u] = record;
            found = record;
            break;
        }
    }
    (void)SceneEvfRecord::Cleanup(query, runtime);
    return found;
}

int PalEventVolumnManager::LoadSceneEvfRecords(
    PalEventVolumnState& state, const std::uint32_t manager_address,
    const std::string_view prefix, const std::string_view folder,
    const std::string_view scene, PalEventVolumnLoadServices services) {
    std::string path;
    path.reserve(prefix.size() + folder.size() + 2u * scene.size() + 7u);
    path.append(prefix);
    path.append(folder);
    path.push_back('\\');
    path.append(scene);
    path.push_back('\\');
    path.append(scene);
    path.append(".evf");

    (void)ClearRecords(state, services.cleanup);
    const auto context = services.scene.OpenStream(path, 2u, 1u);
    if (context != 0u) {
        const auto count = services.scene.ReadRecordCount(context);
        for (std::uint32_t index = 0u; index < count; ++index) {
            const auto address = services.scene.AllocateRecordBytes(172u);
            if (address == 0u) break;
            auto& record = services.scene.RecordAt(address);
            (void)SceneEvfRecord::Initialize(
                record, address, manager_address, services.record);
            if (SceneEvfRecord::ReadFromStream(
                    record, address, context, services.stream, services.type,
                    services.geometry, services.atomic)) {
                services.scene.InsertRecord(state, address);
                ++state.words[5u];
            }
        }
        services.scene.CloseStream(context, 0u);
    }
    return services.scene.FinishTemporaryPath();
}

bool PalEventVolumnManager::InsertTreeEntry(
    PalEventVolumnState& state, const std::uint32_t key,
    const std::uint32_t value, IPalEventVolumnTreeRuntime& runtime) {
    const auto head = state.words[10u];
    const auto nil = runtime.SharedNil();
    auto child = runtime.ReadWord(head + 4u);
    auto parent = head;
    bool insert_left = true;
    const auto signed_key = std::bit_cast<std::int32_t>(key);
    while (child != nil) {
        parent = child;
        insert_left = signed_key < std::bit_cast<std::int32_t>(
            runtime.ReadWord(child + 12u));
        child = runtime.ReadWord(child + (insert_left ? 0u : 8u));
    }

    if ((state.words[11u] & 0xffu) != 0u) {
        runtime.InsertAt(state, child, parent, key, value);
        return true;
    }

    auto candidate = parent;
    if (insert_left) {
        if (parent == runtime.ReadWord(head)) {
            runtime.InsertAt(state, child, parent, key, value);
            return true;
        }
        candidate = PredecessorTreeNode(parent, runtime);
    }
    if (std::bit_cast<std::int32_t>(runtime.ReadWord(candidate + 12u)) >=
        signed_key)
        return false;
    runtime.InsertAt(state, child, parent, key, value);
    return true;
}

std::uint32_t PalEventVolumnManager::FindTreeValue(
    const PalEventVolumnState& state, const std::uint32_t key,
    IPalEventVolumnTreeRuntime& runtime) {
    const auto head = state.words[10u];
    const auto candidate = LowerBoundTreeNode(state, key, runtime);
    if (candidate == head ||
        std::bit_cast<std::int32_t>(key) <
            std::bit_cast<std::int32_t>(runtime.ReadWord(candidate + 12u)))
        return 0u;
    return runtime.ReadWord(candidate + 16u);
}

std::uint32_t PalEventVolumnManager::LowerBoundTreeNode(
    const PalEventVolumnState& state, const std::uint32_t key,
    IPalEventVolumnTreeRuntime& runtime) {
    const auto head = state.words[10u];
    const auto nil = runtime.SharedNil();
    auto candidate = head;
    auto node = runtime.ReadWord(head + 4u);
    const auto signed_key = std::bit_cast<std::int32_t>(key);
    while (node != nil) {
        const auto node_key = std::bit_cast<std::int32_t>(
            runtime.ReadWord(node + 12u));
        if (node_key >= signed_key) {
            candidate = node;
            node = runtime.ReadWord(node);
        } else {
            node = runtime.ReadWord(node + 8u);
        }
    }
    return candidate;
}

int PalEventVolumnManager::CleanupTree(
    PalEventVolumnState& state, PalEventVolumnSharedTree& shared,
    IPalEventVolumnTreeCleanupRuntime& runtime) {
    state.words[8u] = 0x84b490u;
    const auto head = state.words[10u];
    const auto first = runtime.ReadWord(head);
    if (state.words[12u] != 0u) {
        (void)DestroyTreeSubtree(runtime.ReadWord(head + 4u), runtime);
        runtime.WriteWord(head + 4u, shared.nil);
        state.words[12u] = 0u;
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        runtime.ObserveFirstElement(head);
    } else if (first != head) {
        auto node = first;
        do {
            const auto current = node;
            AdvanceTreeCursor(node, runtime);
            runtime.EraseNode(current);
        } while (node != head);
    }
    (void)runtime.Free(head);
    state.words[10u] = 0u;
    state.words[12u] = 0u;
    if (--shared.references != 0u)
        return static_cast<int>(shared.references);
    const auto nil = shared.nil;
    shared.nil = 0u;
    return nil != 0u ? runtime.Free(nil) : 0;
}

std::uint32_t PalEventVolumnManager::PredecessorTreeNode(
    std::uint32_t node, IPalEventVolumnTreeRuntime& runtime) {
    const auto parent = runtime.ReadWord(node + 4u);
    if (runtime.ReadWord(node + 20u) == 0u &&
        runtime.ReadWord(parent + 4u) == node)
        return runtime.ReadWord(node + 8u);

    const auto nil = runtime.SharedNil();
    auto left = runtime.ReadWord(node);
    if (left != nil) {
        while (runtime.ReadWord(left + 8u) != nil)
            left = runtime.ReadWord(left + 8u);
        return left;
    }
    auto ancestor = parent;
    while (node == runtime.ReadWord(ancestor)) {
        node = ancestor;
        ancestor = runtime.ReadWord(ancestor + 4u);
    }
    return ancestor;
}

}  // namespace pal4
