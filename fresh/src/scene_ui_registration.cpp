#include "pal4/scene_ui_registration.h"

namespace pal4 {

int SceneUiRegistration::InitializeGuard(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return CrtExitRegistration::RegisterNullSub(guard, api);
}

std::uint32_t SceneUiRegistration::RegisterName(
    std::uint32_t& name_cache) noexcept {
    name_cache = kNameLiteral;
    return name_cache;
}

std::uint32_t SceneUiRegistration::RegisterNameThunk(
    std::uint32_t& name_cache) noexcept {
    return RegisterName(name_cache);
}

std::uint32_t SceneUiRegistration::CreateObject(
    const std::uint32_t name, ISceneUiRegistrationRuntime& runtime) {
    const auto storage = runtime.Allocate(0xf8u);
    if (storage == 0u) return 0u;
    try {
        return runtime.Construct(storage, name);
    } catch (...) {
        runtime.Free(storage);
        throw;
    }
}

std::uint32_t SceneUiRegistration::RegisterFactory(
    const std::uint32_t registered_name,
    ISceneUiRegistrationRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    return runtime.ProcessFactory(manager, registered_name, &CreateObject);
}

std::uint32_t SceneUiRegistration::RegisterFactoryThunk(
    const std::uint32_t registered_name,
    ISceneUiRegistrationRuntime& runtime) {
    return RegisterFactory(registered_name, runtime);
}

std::uint32_t SceneUiRegistration::RegisterShowCommand(
    ISceneUiRegistrationRuntime& runtime) {
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

std::uint32_t SceneUiRegistration::RegisterShowCommandThunk(
    ISceneUiRegistrationRuntime& runtime) {
    return RegisterShowCommand(runtime);
}

}  // namespace pal4
