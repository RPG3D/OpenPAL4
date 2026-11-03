#include "pal4/game_arena_object_list.h"

namespace pal4 {
namespace {

constexpr std::uint32_t kSlotBase = 0x1fcu;
constexpr std::uint32_t kSlotStride = 36u;
constexpr std::uint32_t kSlotCount = 3u;

std::uint32_t Slot(std::uint32_t arena, std::uint32_t index) {
    return arena + kSlotBase + index * kSlotStride;
}

bool IsWorldAvailable(std::uint32_t arena,
                      IGameArenaObjectListRuntime& runtime) {
    const auto world = runtime.ReadWord(arena + 0x108u);
    return world != 0u && runtime.ReadWord(world + 0x34u) != 1u;
}

void StoreNames(std::uint32_t arena, std::uint32_t slot,
                IGameArenaObjectListRuntime& runtime) {
    runtime.InitializeText(
        slot, runtime.ReadCString(runtime.ReadWord(arena + 8u)));
    runtime.InitializeText(
        slot + 12u, runtime.ReadCString(runtime.ReadWord(arena + 12u)));
}

}  // namespace

std::uint32_t GameArenaObjectList::FindByName(
    std::uint32_t arena, IGameArenaObjectListRuntime& runtime) {
    if (!IsWorldAvailable(arena, runtime)) return 0u;

    for (std::uint32_t index = 0; index < kSlotCount; ++index) {
        const auto slot = Slot(arena, index);
        if (runtime.ReadWord(slot + 24u) == 0u) continue;

        const auto worldName =
            runtime.ReadCString(runtime.ReadWord(arena + 8u));
        if (!runtime.TextEqualsIgnoreCase(slot, worldName)) continue;

        const auto areaName =
            runtime.ReadCString(runtime.ReadWord(arena + 12u));
        if (!runtime.TextEqualsIgnoreCase(slot + 12u, areaName)) continue;

        runtime.WriteWord(slot + 28u, 1u);
        return runtime.ReadWord(slot + 32u);
    }
    return 0u;
}

void GameArenaObjectList::AddPending(
    std::uint32_t arena, IGameArenaObjectListRuntime& runtime) {
    const auto pending = runtime.ReadWord(arena + 0x194u);
    if (pending == 0u || !IsWorldAvailable(arena, runtime)) return;

    for (std::uint32_t index = 0; index < kSlotCount; ++index) {
        const auto slot = Slot(arena, index);
        if (runtime.ReadWord(slot + 24u) == 0u &&
            runtime.ReadWord(slot + 28u) == 0u) {
            StoreNames(arena, slot, runtime);
            runtime.WriteWord(slot + 24u, 1u);
            runtime.WriteWord(slot + 32u, runtime.ReadWord(arena + 0x194u));
            runtime.WriteWord(arena + 0x194u, 0u);
            return;
        }
        if (runtime.ReadWord(slot + 32u) == pending) {
            runtime.WriteWord(arena + 0x194u, 0u);
            return;
        }
    }

    std::uint32_t index = 0;
    while (index < kSlotCount &&
           runtime.ReadWord(Slot(arena, index) + 32u) == pending) {
        ++index;
    }
    if (index == kSlotCount) return;

    const auto slot = Slot(arena, index);
    runtime.ReleaseObjectChildren(runtime.ReadWord(slot + 32u));
    const auto oldObject = runtime.ReadWord(slot + 32u);
    if (oldObject != 0u) {
        runtime.DestroyObject(oldObject);
        static_cast<void>(runtime.FreeObject(oldObject));
    }
    StoreNames(arena, slot, runtime);
    runtime.WriteWord(slot + 24u, 1u);
    runtime.WriteWord(slot + 28u, 0u);
    runtime.WriteWord(slot + 32u, runtime.ReadWord(arena + 0x194u));
    runtime.WriteWord(arena + 0x194u, 0u);
}

std::uint32_t GameArenaObjectList::Initialize(
    std::uint32_t arena, IGameArenaObjectListRuntime& runtime) {
    for (std::uint32_t index = 0; index < kSlotCount; ++index) {
        const auto slot = Slot(arena, index);
        runtime.WriteWord(slot + 24u, 0u);
        runtime.WriteWord(slot + 28u, 0u);
        runtime.WriteWord(slot + 32u, 0u);
    }
    return arena + 0x284u;
}

std::uint32_t GameArenaObjectList::Cleanup(
    std::uint32_t arena, IGameArenaObjectListRuntime& runtime) {
    std::uint32_t result = 0u;
    for (std::uint32_t index = 0; index < kSlotCount; ++index) {
        const auto slot = Slot(arena, index);
        if (runtime.ReadWord(slot + 24u) == 0u) continue;
        const auto object = runtime.ReadWord(slot + 32u);
        if (object == 0u) continue;
        runtime.DestroyObject(object);
        result = runtime.FreeObject(object);
        runtime.WriteWord(slot + 32u, 0u);
        runtime.WriteWord(slot + 24u, 0u);
        runtime.WriteWord(slot + 28u, 0u);
    }
    return result;
}

}  // namespace pal4
