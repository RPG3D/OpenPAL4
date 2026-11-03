#include "pal4/role_state_window_registration.h"

namespace pal4 {

int RoleStateWindowRegistration::InitializeGuard(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return CrtExitRegistration::RegisterNullSub(guard, api);
}

std::uint32_t RoleStateWindowRegistration::RegisterName(
    std::uint32_t& name_cache) noexcept {
    name_cache = kNameLiteral;
    return name_cache;
}

std::uint32_t RoleStateWindowRegistration::RegisterNameThunk(
    std::uint32_t& name_cache) noexcept {
    return RegisterName(name_cache);
}

std::uint32_t RoleStateWindowRegistration::CreateObject(
    const std::uint32_t name,
    IRoleStateWindowRegistrationRuntime& runtime) {
    const auto storage = runtime.Allocate(0x258u);
    if (storage == 0u) return 0u;
    try {
        return runtime.Construct(storage, name);
    } catch (...) {
        runtime.Free(storage);
        throw;
    }
}

std::uint32_t RoleStateWindowRegistration::RegisterFactory(
    const std::uint32_t registered_name,
    IRoleStateWindowRegistrationRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    return runtime.ProcessFactory(manager, registered_name, &CreateObject);
}

std::uint32_t RoleStateWindowRegistration::RegisterFactoryThunk(
    const std::uint32_t registered_name,
    IRoleStateWindowRegistrationRuntime& runtime) {
    return RegisterFactory(registered_name, runtime);
}

std::uint32_t RoleStateWindowRegistration::RegisterShowCommand(
    IRoleStateWindowRegistrationRuntime& runtime) {
    auto callback = runtime.Allocate(8u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 4u, kNameLiteral);
        runtime.WriteWord(callback, kCallbackVtable);
    }
    try {
        runtime.InsertOrReplace(runtime.FrameManager() + 0x104u,
                                kCommandLiteral, callback);
    } catch (...) {
        if (callback != 0u) runtime.DeleteCallback(callback, 1u);
        throw;
    }
    if (callback != 0u) runtime.DeleteCallback(callback, 1u);
    return kCommandResult;
}

std::uint32_t RoleStateWindowRegistration::RegisterShowCommandThunk(
    IRoleStateWindowRegistrationRuntime& runtime) {
    return RegisterShowCommand(runtime);
}

}  // namespace pal4
