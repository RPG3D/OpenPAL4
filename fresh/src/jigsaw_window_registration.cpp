#include "pal4/jigsaw_window_registration.h"

namespace pal4 {

std::uint32_t JigsawWindowRegistration::InitializeNameThunk(
    IJigsawWindowRegistrationRuntime& runtime) {
    return InitializeName(runtime);
}

std::uint32_t JigsawWindowRegistration::InitializeName(
    IJigsawWindowRegistrationRuntime& runtime) {
    const auto name = runtime.ReadWord(0x8b123cu);
    runtime.WriteWord(0x8eac80u, name);
    return name;
}

std::uint32_t JigsawWindowRegistration::Create(
    const std::uint32_t layout_name,
    IJigsawWindowRegistrationRuntime& runtime) {
    const auto object = runtime.Allocate(0x190u);
    if (object == 0u) return 0u;
    try {
        return runtime.Construct(object, layout_name);
    } catch (...) {
        runtime.Free(object);
        throw;
    }
}

std::uint32_t JigsawWindowRegistration::RegisterFactoryThunk(
    IJigsawWindowRegistrationRuntime& runtime) {
    return RegisterFactory(runtime);
}

std::uint32_t JigsawWindowRegistration::RegisterFactory(
    IJigsawWindowRegistrationRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    const auto name = runtime.ReadWord(0x8eac80u);
    return runtime.RegisterFactory(manager, name, 0x49f750u);
}

std::uint32_t JigsawWindowRegistration::RegisterHandlerThunk(
    IJigsawWindowRegistrationRuntime& runtime) {
    return RegisterHandler(runtime);
}

std::uint32_t JigsawWindowRegistration::RegisterHandler(
    IJigsawWindowRegistrationRuntime& runtime) {
    const auto handler = runtime.Allocate(8u);
    if (handler != 0u) {
        runtime.WriteWord(handler + 4u, 0x49fed0u);
        runtime.WriteWord(handler, 0x8435dcu);
    }
    try {
        runtime.InsertOrReplace(runtime.FrameManager() + 0x104u,
                                0x8b1238u, handler);
    } catch (...) {
        if (handler != 0u) runtime.DestroyHandler(handler, 1u);
        throw;
    }
    if (handler != 0u) runtime.DestroyHandler(handler, 1u);
    return 0x8eac7cu;
}

}  // namespace pal4
