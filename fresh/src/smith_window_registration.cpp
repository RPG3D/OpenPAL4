#include "pal4/smith_window_registration.h"

namespace pal4 {

std::uint32_t SmithWindowRegistration::InitializeThunk(
    ISmithWindowRegistrationRuntime& runtime) {
    return Initialize(runtime);
}

std::uint32_t SmithWindowRegistration::Initialize(
    ISmithWindowRegistrationRuntime& runtime) {
    const auto name = runtime.ReadWord(0x8b0f20u);
    runtime.WriteWord(0x8eac6cu, name);
    return name;
}

std::uint32_t SmithWindowRegistration::RegisterUIThunk(
    ISmithWindowRegistrationRuntime& runtime) {
    return RegisterUI(runtime);
}

std::uint32_t SmithWindowRegistration::RegisterUI(
    ISmithWindowRegistrationRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    const auto name = runtime.ReadWord(0x8eac6cu);
    return runtime.RegisterFactory(manager, name, 0x496ae0u);
}

std::uint32_t SmithWindowRegistration::RegisterEventHandlerThunk(
    ISmithWindowRegistrationRuntime& runtime) {
    return RegisterEventHandler(runtime);
}

std::uint32_t SmithWindowRegistration::RegisterEventHandler(
    ISmithWindowRegistrationRuntime& runtime) {
    const auto name = runtime.ReadWord(0x8b0f20u);
    const auto handler = runtime.Allocate(8u);
    if (handler != 0u) {
        runtime.WriteWord(handler + 4u, name);
        runtime.WriteWord(handler, 0x843494u);
    }
    try {
        runtime.InsertOrReplace(runtime.FrameManager() + 0x104u,
                                0x8b0f1cu, handler);
    } catch (...) {
        if (handler != 0u) runtime.DestroyHandler(handler, 1u);
        throw;
    }
    if (handler != 0u) runtime.DestroyHandler(handler, 1u);
    return 0x8eac68u;
}

}  // namespace pal4
