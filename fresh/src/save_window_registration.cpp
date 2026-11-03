#include "pal4/save_window_registration.h"

namespace pal4 {

const char* SaveWindowRegistration::RegisterName(
    const char*& destination) noexcept {
    destination = "SaveWindow";
    return destination;
}

const char* SaveWindowRegistration::RegisterNameThunk(
    const char*& destination) noexcept {
    return RegisterName(destination);
}

std::uint32_t SaveWindowRegistration::CreateObject(
    std::uint32_t name, ISaveWindowRegistrationRuntime& runtime) {
    const auto storage = runtime.Allocate(0x64u);
    if (storage == 0u) return 0u;
    try {
        return runtime.Construct(storage, name);
    } catch (...) {
        runtime.Free(storage);
        throw;
    }
}

std::uint32_t SaveWindowRegistration::RegisterFactory(
    ISaveWindowRegistrationRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    return runtime.RegisterFactory(manager, runtime.RegisteredName(),
                                   &CreateObject);
}

std::uint32_t SaveWindowRegistration::RegisterFactoryThunk(
    ISaveWindowRegistrationRuntime& runtime) {
    return RegisterFactory(runtime);
}

std::uint32_t SaveWindowRegistration::RegisterShowCommand(
    ISaveWindowRegistrationRuntime& runtime) {
    const auto name = runtime.LiteralName();
    auto callback = runtime.Allocate(8u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 4u, name);
        runtime.WriteWord(callback, kCommandCallbackVtable);
    }
    try {
        const auto manager = runtime.FrameManager();
        runtime.InsertOrReplace(manager + 0x104u, "show_save_ui",
                                callback);
    } catch (...) {
        if (callback != 0u) runtime.DeleteCallback(callback, 1u);
        throw;
    }
    if (callback != 0u) runtime.DeleteCallback(callback, 1u);
    return kCommandResultAddress;
}

std::uint32_t SaveWindowRegistration::RegisterShowCommandThunk(
    ISaveWindowRegistrationRuntime& runtime) {
    return RegisterShowCommand(runtime);
}

}  // namespace pal4
