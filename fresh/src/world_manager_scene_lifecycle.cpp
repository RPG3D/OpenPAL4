#include "pal4/world_manager_scene_lifecycle.h"

#include <initializer_list>

namespace pal4 {

std::uint32_t WorldManagerSceneLifecycle::Construct(
    const std::uint32_t object, IWorldManagerSceneRuntime& runtime) {
    runtime.ConstructBaseWorld(object);
    runtime.WriteByte(object + 196u, 0u);
    for (const auto offset : {200u, 204u, 208u})
        runtime.WriteWord(object + offset, 0u);
    runtime.WriteByte(object + 212u, 0u);
    for (const auto offset : {216u, 220u, 224u, 228u})
        runtime.WriteWord(object + offset, 0u);
    runtime.WriteWord(object, kVtable);
    return object;
}

std::uint32_t WorldManagerSceneLifecycle::DeletingDestructor(
    const std::uint32_t object, const std::uint32_t flags,
    IWorldManagerSceneRuntime& runtime) {
    Destroy(object, runtime);
    if ((flags & 1u) != 0u) runtime.Free(object);
    return object;
}

void WorldManagerSceneLifecycle::Destroy(
    const std::uint32_t object, IWorldManagerSceneRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    runtime.Free(runtime.ReadWord(object + 216u));
    for (const auto offset : {216u, 220u, 224u})
        runtime.WriteWord(object + offset, 0u);
    runtime.Free(runtime.ReadWord(object + 200u));
    for (const auto offset : {200u, 204u, 208u})
        runtime.WriteWord(object + offset, 0u);
    runtime.DestroyBaseWorld(object);
}

}  // namespace pal4
