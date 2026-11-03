#include "pal4/pal_test_window_registration.h"

namespace pal4 {

const char* PalTestWindowRegistration::RegisterName(
    const char*& destination) noexcept {
    destination = "PalTestWindow";
    return destination;
}

const char* PalTestWindowRegistration::RegisterNameThunk(
    const char*& destination) noexcept {
    return RegisterName(destination);
}

std::uint32_t PalTestWindowRegistration::CreateObject(
    std::uint32_t name, IPalTestWindowRegistrationRuntime& runtime) {
    const auto storage = runtime.Allocate(0x8cu);
    if (storage == 0u) return 0u;
    try {
        return runtime.Construct(storage, name);
    } catch (...) {
        runtime.Free(storage);
        throw;
    }
}

std::uint32_t PalTestWindowRegistration::RegisterFactory(
    IPalTestWindowRegistrationRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    return runtime.RegisterFactory(manager, runtime.RegisteredName(),
                                   &CreateObject);
}

std::uint32_t PalTestWindowRegistration::RegisterFactoryThunk(
    IPalTestWindowRegistrationRuntime& runtime) {
    return RegisterFactory(runtime);
}

std::uint32_t PalTestWindowRegistration::RegisterShowCommand(
    IPalTestWindowRegistrationRuntime& runtime) {
    const auto name = runtime.LiteralName();
    auto callback = runtime.Allocate(8u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 4u, name);
        runtime.WriteWord(callback, kCommandCallbackVtable);
    }
    try {
        const auto manager = runtime.FrameManager();
        runtime.InsertOrReplace(manager + 0x104u, "show_test_ui",
                                callback);
    } catch (...) {
        if (callback != 0u) runtime.DeleteCallback(callback, 1u);
        throw;
    }
    if (callback != 0u) runtime.DeleteCallback(callback, 1u);
    return kCommandResultAddress;
}

std::uint32_t PalTestWindowRegistration::RegisterShowCommandThunk(
    IPalTestWindowRegistrationRuntime& runtime) {
    return RegisterShowCommand(runtime);
}

}  // namespace pal4
