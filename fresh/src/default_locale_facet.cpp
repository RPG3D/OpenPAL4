#include "pal4/default_locale_facet.h"

namespace pal4 {

void DefaultLocaleFacet::Set(IDefaultLocaleFacetRuntime& runtime) {
    runtime.Write(kGlobalAddress, kVtableAddress);
}

void DefaultLocaleFacet::Reset(IDefaultLocaleFacetRuntime& runtime) {
    runtime.Write(kGlobalAddress, kVtableAddress);
}

std::uint32_t DefaultLocaleFacet::InitializeAndRegisterExit(
    IDefaultLocaleFacetRuntime& runtime) {
    Set(runtime);
    return runtime.RegisterExitHandler(kResetEntry);
}

std::uint32_t LocaleFacetLifecycle::Destroy(
    const std::uint32_t object, const std::uint32_t flags,
    IDefaultLocaleFacetRuntime& runtime) {
    runtime.Write(object, DefaultLocaleFacet::kVtableAddress);
    if ((flags & 1u) != 0u) {
        runtime.Free(object);
    }
    return object;
}

void ActorMessageLocaleFacet::SetVtable(
    const std::uint32_t object, IDefaultLocaleFacetRuntime& runtime) {
    runtime.Write(object, kVtableAddress);
}

std::uint32_t ActorMessageLocaleFacet::Destroy(
    const std::uint32_t object, const std::uint32_t flags,
    IDefaultLocaleFacetRuntime& runtime) {
    SetVtable(object, runtime);
    if ((flags & 1u) != 0u) runtime.Free(object);
    return object;
}

void SystemEventLocaleFacet::ResetBase(
    const std::uint32_t object, IDefaultLocaleFacetRuntime& runtime) {
    runtime.Write(object, kBaseVtable);
}

std::uint32_t SystemEventLocaleFacet::DestroyBase(
    const std::uint32_t object, const std::uint32_t flags,
    IDefaultLocaleFacetRuntime& runtime) {
    ResetBase(object, runtime);
    if (flags & 1u) runtime.Free(object);
    return object;
}

std::uint32_t SystemEventLocaleFacet::InitializeData(
    const std::uint32_t object, IDefaultLocaleFacetRuntime& runtime) {
    CleanupData(object, runtime);
    return object;
}

void SystemEventLocaleFacet::CleanupData(
    const std::uint32_t object, IDefaultLocaleFacetRuntime& runtime) {
    runtime.Write(object, kDataVtable);
    ResetData(object, runtime);
}

void SystemEventLocaleFacet::ResetData(
    const std::uint32_t object, IDefaultLocaleFacetRuntime& runtime) {
    runtime.Write(object + 4u, 0);
}

void ActorManagerLocaleFacet::Reset(
    const std::uint32_t object, IDefaultLocaleFacetRuntime& runtime) {
    runtime.Write(object, kVtableAddress);
}

std::uint32_t ActorManagerLocaleFacet::Destroy(
    const std::uint32_t object, const std::uint32_t flags,
    IDefaultLocaleFacetRuntime& runtime) {
    Reset(object, runtime);
    if ((flags & 1u) != 0u) runtime.Free(object);
    return object;
}

}  // namespace pal4
