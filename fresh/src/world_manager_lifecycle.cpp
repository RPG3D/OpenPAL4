#include "pal4/world_manager_lifecycle.h"

#include <initializer_list>

namespace pal4 {

std::uint32_t WorldManagerLifecycle::Construct(
    const std::uint32_t manager, IWorldManagerLifecycleRuntime& runtime) {
    runtime.ConstructBaseManager(manager);
    for (std::uint32_t offset = 84u; offset <= 112u; offset += 4u)
        runtime.WriteWord(manager + offset, 0u);

    bool scene_constructed = false;
    bool texts_constructed = false;
    try {
        (void)WorldManagerSceneLifecycle::Construct(manager + 116u, runtime);
        scene_constructed = true;
        runtime.WriteByte(manager + 348u, 0u);
        for (const auto offset : {352u, 356u, 360u})
            runtime.WriteWord(manager + offset, 0u);
        runtime.WriteByte(manager + 364u, 0u);
        for (const auto offset : {368u, 372u, 376u})
            runtime.WriteWord(manager + offset, 0u);
        texts_constructed = true;
        (void)WorldManagerEventSubscriptions::Subscribe(manager, runtime);
    } catch (...) {
        if (texts_constructed) {
            ReleaseText(manager + 364u, runtime);
            ReleaseText(manager + 348u, runtime);
        }
        if (scene_constructed)
            WorldManagerSceneLifecycle::Destroy(manager + 116u, runtime);
        runtime.DestroyBaseManager(manager);
        throw;
    }
    return manager;
}

void WorldManagerLifecycle::ReleaseText(
    const std::uint32_t descriptor, IWorldManagerLifecycleRuntime& runtime) {
    const auto text = runtime.ReadWord(descriptor + 4u);
    if (text != 0u) {
        const auto header = text - 1u;
        const auto refs = runtime.ReadByte(header);
        if (refs == 0u || refs == 0xffu)
            runtime.FreeTextStorage(header);
        else
            runtime.WriteByte(header, static_cast<std::uint8_t>(refs - 1u));
    }
    for (const auto offset : {4u, 8u, 12u})
        runtime.WriteWord(descriptor + offset, 0u);
}

void WorldManagerLifecycle::Destroy(
    const std::uint32_t manager, IWorldManagerLifecycleRuntime& runtime) {
    runtime.CleanupManager(manager);
    ReleaseText(manager + 364u, runtime);
    ReleaseText(manager + 348u, runtime);
    WorldManagerSceneLifecycle::Destroy(manager + 116u, runtime);
    runtime.DestroyBaseManager(manager);
}

std::uint32_t WorldManagerLifecycle::ActorData(
    const std::uint32_t manager) {
    return manager + 328u;
}

}  // namespace pal4
