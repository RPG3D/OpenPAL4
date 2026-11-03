#include "pal4/manager_object_lifecycle.h"

#include <array>

namespace pal4 {
namespace {

constexpr std::array<std::uint32_t, 15> kCommonVtables{
    0x842540u, 0x842500u, 0x8424c0u, 0x842480u, 0x842440u,
    0u, 0x842380u, 0u, 0x842300u, 0x8422c0u,
    0x842280u, 0x842240u, 0x842200u, 0u, 0x842180u};

}  // namespace

void ManagerObjectLifecycleRuntimeStub::WriteVtable(
    std::uint32_t /*object*/, std::uint32_t /*vtable*/) {}
std::uint8_t ManagerObjectLifecycleRuntimeStub::Cleanup(
    std::uint32_t /*object*/) { return 0u; }
void ManagerObjectLifecycleRuntimeStub::CleanupEffect(
    std::uint32_t /*object*/) {}
void ManagerObjectLifecycleRuntimeStub::CleanupExtended(
    std::uint32_t /*object*/) {}
std::uint8_t ManagerObjectLifecycleRuntimeStub::CleanupSecondary(
    std::uint32_t /*object*/) { return 0u; }
std::uint32_t ManagerObjectLifecycleRuntimeStub::ReadDword(
    std::uint32_t /*address*/) { return 0u; }
void ManagerObjectLifecycleRuntimeStub::WriteDword(
    std::uint32_t /*address*/, std::uint32_t /*value*/) {}
void ManagerObjectLifecycleRuntimeStub::Free(std::uint32_t /*object*/) {}

std::uint32_t ManagerObjectLifecycle::DeleteBase(
    const std::uint32_t object, const std::uint8_t flags,
    IManagerObjectLifecycleRuntime& runtime) {
    runtime.WriteVtable(object, 0x842400u);
    if ((flags & 1u) != 0u) runtime.Free(object);
    return object;
}

std::uint8_t ManagerObjectLifecycle::DestroyFirst(
    const std::uint32_t object, IManagerObjectLifecycleRuntime& runtime) {
    return DestroyCommon(object, 1u, runtime);
}

std::uint8_t ManagerObjectLifecycle::DestroyCommon(
    const std::uint32_t object, const std::uint32_t manager_key,
    IManagerObjectLifecycleRuntime& runtime) {
    if (manager_key == 0u || manager_key > kCommonVtables.size() ||
        kCommonVtables[manager_key - 1u] == 0u) {
        return 0u;
    }
    runtime.WriteVtable(object, kCommonVtables[manager_key - 1u]);
    const auto result = runtime.Cleanup(object);
    runtime.WriteVtable(object, 0x842400u);
    return result;
}

std::uint32_t ManagerObjectLifecycle::DeleteFirst(
    const std::uint32_t object, const std::uint8_t flags,
    IManagerObjectLifecycleRuntime& runtime) {
    return DeleteCommon(object, 1u, flags, runtime);
}

std::uint32_t ManagerObjectLifecycle::DeleteCommon(
    const std::uint32_t object, const std::uint32_t manager_key,
    const std::uint8_t flags, IManagerObjectLifecycleRuntime& runtime) {
    (void)DestroyCommon(object, manager_key, runtime);
    if ((flags & 1u) != 0u) {
        runtime.Free(object);
    }
    return object;
}

void ManagerObjectLifecycle::DestroyEffect(
    const std::uint32_t object, IManagerObjectLifecycleRuntime& runtime) {
    runtime.WriteVtable(object, 0x842340u);
    runtime.CleanupEffect(object);
    runtime.WriteVtable(object, 0x842400u);
}

std::uint32_t ManagerObjectLifecycle::DeleteEffect(
    const std::uint32_t object, const std::uint8_t flags,
    IManagerObjectLifecycleRuntime& runtime) {
    DestroyEffect(object, runtime);
    if ((flags & 1u) != 0u) {
        runtime.Free(object);
    }
    return object;
}

std::uint32_t ManagerObjectLifecycle::DestroyExtended(
    const std::uint32_t object, IManagerObjectLifecycleRuntime& runtime) {
    runtime.WriteVtable(object, 0x8423c0u);
    runtime.CleanupExtended(object);
    runtime.Free(runtime.ReadDword(object + 36u));
    runtime.WriteDword(object + 36u, 0u);
    runtime.WriteDword(object + 40u, 0u);
    runtime.WriteDword(object + 44u, 0u);
    runtime.WriteVtable(object, 0x842400u);
    return 0u;
}

std::uint32_t ManagerObjectLifecycle::DeleteExtended(
    const std::uint32_t object, const std::uint8_t flags,
    IManagerObjectLifecycleRuntime& runtime) {
    (void)DestroyExtended(object, runtime);
    if ((flags & 1u) != 0u) {
        runtime.Free(object);
    }
    return object;
}

std::uint8_t ManagerObjectLifecycle::DestroySecondary(
    const std::uint32_t object, IManagerObjectLifecycleRuntime& runtime) {
    runtime.WriteVtable(object, 0x8421c0u);
    const auto result = runtime.CleanupSecondary(object);
    runtime.WriteVtable(object, 0x842400u);
    return result;
}

std::uint32_t ManagerObjectLifecycle::DeleteSecondary(
    const std::uint32_t object, const std::uint8_t flags,
    IManagerObjectLifecycleRuntime& runtime) {
    (void)DestroySecondary(object, runtime);
    if ((flags & 1u) != 0u) {
        runtime.Free(object);
    }
    return object;
}

std::uint32_t ManagerObjectLifecycle::ConstantTrue() {
    return 1u;
}

}  // namespace pal4
