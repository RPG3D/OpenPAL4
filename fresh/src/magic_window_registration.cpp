#include "pal4/magic_window_registration.h"

namespace pal4 {

const char* MagicWindowRegistration::RegisterName(const char*& destination) noexcept {
    destination = "magicWindow";
    return destination;
}

const char* MagicWindowRegistration::RegisterNameThunk(const char*& destination) noexcept {
    return RegisterName(destination);
}

std::uint32_t MagicWindowRegistration::CreateObject(
    std::uint32_t name, IMagicWindowRegistrationRuntime& runtime) {
    const auto storage = runtime.Allocate(0x1e0u);
    if (storage == 0) return 0;
    try {
        return runtime.Construct(storage, name);
    } catch (...) {
        runtime.Free(storage);
        throw;
    }
}

std::uint32_t MagicWindowRegistration::RegisterFactory(
    IMagicWindowRegistrationRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    return runtime.RegisterFactory(manager, runtime.CachedName(), &CreateObject);
}

std::uint32_t MagicWindowRegistration::RegisterFactoryThunk(
    IMagicWindowRegistrationRuntime& runtime) {
    return RegisterFactory(runtime);
}

std::uint32_t MagicWindowRegistration::RegisterShowCommand(
    IMagicWindowRegistrationRuntime& runtime) {
    const auto name = runtime.LiteralName();
    auto callback = runtime.Allocate(8u);
    if (callback != 0) {
        runtime.WriteWord(callback + 4u, name);
        runtime.WriteWord(callback, kCommandCallbackVtable);
    }
    try {
        const auto manager = runtime.FrameManager();
        runtime.InsertOrReplace(manager + 0x104u, "show_magic_ui", callback);
    } catch (...) {
        if (callback != 0) runtime.DeleteCallback(callback, 1u);
        throw;
    }
    if (callback != 0) runtime.DeleteCallback(callback, 1u);
    return kCommandResultAddress;
}

std::uint32_t MagicWindowRegistration::RegisterShowCommandThunk(
    IMagicWindowRegistrationRuntime& runtime) {
    return RegisterShowCommand(runtime);
}

}  // namespace pal4
