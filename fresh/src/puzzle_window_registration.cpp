#include "pal4/puzzle_window_registration.h"

namespace pal4 {

std::uint32_t PuzzleWindowRegistration::InitializeNameThunk(
    IPuzzleWindowRegistrationRuntime& runtime) {
    return InitializeName(runtime);
}

std::uint32_t PuzzleWindowRegistration::InitializeName(
    IPuzzleWindowRegistrationRuntime& runtime) {
    const auto name = runtime.ReadWord(0x8b12dcu);
    runtime.WriteWord(0x8eac88u, name);
    return name;
}

std::uint32_t PuzzleWindowRegistration::Create(
    const std::uint32_t layout_name,
    IPuzzleWindowRegistrationRuntime& runtime) {
    const auto object = runtime.Allocate(0x1a4u);
    if (object == 0u) return 0u;
    try {
        return runtime.Construct(object, layout_name);
    } catch (...) {
        runtime.Free(object);
        throw;
    }
}

std::uint32_t PuzzleWindowRegistration::RegisterFactoryThunk(
    IPuzzleWindowRegistrationRuntime& runtime) {
    return RegisterFactory(runtime);
}

std::uint32_t PuzzleWindowRegistration::RegisterFactory(
    IPuzzleWindowRegistrationRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    return runtime.RegisterFactory(manager,
                                   runtime.ReadWord(0x8eac88u),
                                   0x4a0a40u);
}

std::uint32_t PuzzleWindowRegistration::RegisterHandlerThunk(
    IPuzzleWindowRegistrationRuntime& runtime) {
    return RegisterHandler(runtime);
}

std::uint32_t PuzzleWindowRegistration::RegisterHandler(
    IPuzzleWindowRegistrationRuntime& runtime) {
    const auto handler = runtime.Allocate(8u);
    if (handler != 0u) {
        runtime.WriteWord(handler + 4u, 0x4a1180u);
        runtime.WriteWord(handler, 0x8435dcu);
    }
    try {
        runtime.InsertOrReplace(runtime.FrameManager() + 0x104u,
                                0x8b12d8u, handler);
    } catch (...) {
        if (handler != 0u) runtime.DestroyHandler(handler, 1u);
        throw;
    }
    if (handler != 0u) runtime.DestroyHandler(handler, 1u);
    return 0x8eac84u;
}

}  // namespace pal4
