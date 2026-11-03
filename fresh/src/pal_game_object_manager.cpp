#include "pal4/pal_game_object_manager.h"

namespace pal4 {

std::uint32_t PalGameObjectManager::Get(
    PalGameObjectManagerSingletonState& state,
    IPalGameObjectManagerRuntime& runtime) {
    if ((state.guard & 1u) != 0u) return state.instance;
    state.guard = static_cast<std::uint8_t>(state.guard | 1u);
    state.instance = runtime.RegisterSingleton(
        runtime.SingletonManager(), "palGameObjectManager", 1045u,
        0x5a6d40u, 0x5a6da0u);
    return state.instance;
}

std::uint32_t PalGameObjectManager::Create(
    IPalGameObjectManagerRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) return 0u;
    try {
        return Construct(object, runtime);
    } catch (...) {
        static_cast<void>(runtime.Free(object));
        throw;
    }
}

std::uint32_t PalGameObjectManager::Destroy(
    const std::uint32_t object, IPalGameObjectManagerRuntime& runtime) {
    if (object == 0u) return 0u;
    static_cast<void>(Destruct(object, runtime));
    return runtime.Free(object);
}

std::uint32_t PalGameObjectManager::Construct(
    const std::uint32_t object, IPalGameObjectManagerRuntime& runtime) {
    runtime.InitializeTree(object, kObjectTreeNodeSize);
    try {
        runtime.InitializeTree(object + 0x14u, kDialogTreeNodeSize);
    } catch (...) {
        static_cast<void>(CleanupObjectTree(object, runtime));
        throw;
    }
    runtime.WriteWord(object + 0x10u, kVtable);
    return object;
}

std::uint32_t PalGameObjectManager::Destruct(
    const std::uint32_t object, IPalGameObjectManagerRuntime& runtime) {
    static_cast<void>(CleanupFactoryTree(object, runtime));
    return CleanupObjectTree(object, runtime);
}

std::uint32_t PalGameObjectManager::CleanupObjectTree(
    const std::uint32_t object, IPalGameObjectManagerRuntime& runtime) {
    return runtime.DestroyTree(object, kObjectTreeNodeSize);
}

bool PalGameObjectManager::RegisterFactory(
    const std::uint32_t object, const std::int32_t type,
    const std::uint32_t factory,
    IPalGameObjectManagerRuntime& runtime) {
    return runtime.InsertFactory(object + 0x14u, type, factory);
}

std::uint32_t PalGameObjectManager::CleanupFactoryTree(
    const std::uint32_t object, IPalGameObjectManagerRuntime& runtime) {
    runtime.WriteWord(object + 0x10u, kVtable);
    return runtime.DestroyTree(object + 0x14u, kDialogTreeNodeSize);
}

}  // namespace pal4
