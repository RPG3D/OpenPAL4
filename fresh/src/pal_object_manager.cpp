#include "pal4/pal_object_manager.h"

namespace pal4 {

std::uint32_t PalObjectManager::RegisterSingleton(
    PalObjectManagerRegistration& registration,
    IPalObjectManagerRuntime& runtime) {
    if ((registration.guard & 1u) != 0u) return registration.cached;
    registration.guard = static_cast<std::uint8_t>(registration.guard | 1u);
    const auto manager = runtime.GetSingletonManager();
    registration.cached = runtime.Register(manager, "palObjectManager",
        kSingletonId, kCreateCallback, kDestroyCallback);
    return registration.cached;
}

std::uint32_t PalObjectManager::Create(IPalObjectManagerRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) return 0u;
    try {
        return Initialize(object, runtime);
    } catch (...) {
        (void)runtime.Free(object);
        throw;
    }
}

std::uint32_t PalObjectManager::Destroy(
    const std::uint32_t object, IPalObjectManagerRuntime& runtime) {
    if (object == 0u) return 0u;
    ForwardDestructor(object, runtime);
    return runtime.Free(object);
}

std::uint32_t PalObjectManager::Initialize(
    const std::uint32_t object, IPalObjectManagerRuntime& runtime) {
    runtime.InitializeBase(object);
    try {
        runtime.ConstructLock(object + 32u);
        runtime.SetReleaseCallback(object, kReleaseCallback);
        runtime.SetCompareCallback(object, kCompareCallback);
        runtime.SetCopyCallback(object, kCopyCallback);
        runtime.SetFreeCallback(object, kFreeCallback);
        runtime.SetName(object, "object-manager-binPtrTree");
    } catch (...) {
        ForwardDestructor(object, runtime);
        throw;
    }
    return object;
}

void PalObjectManager::ForwardDestructor(
    const std::uint32_t object, IPalObjectManagerRuntime& runtime) {
    runtime.DestructorThunk(object);
}

std::uint32_t PalObjectManager::FreeThunk(
    const std::uint32_t object, IPalObjectManagerRuntime& runtime) {
    return runtime.Free(object);
}

}  // namespace pal4
