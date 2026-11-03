#include "pal4/property_window_registration.h"

namespace pal4 {

int PropertyWindowRegistration::InitializeStatic(
    std::uint8_t& flag, ICrtNoopAtExitRuntime& runtime) {
    return CrtNoopAtExitRegistration::Run(flag, runtime);
}

const char* PropertyWindowRegistration::RegisterName(
    const char*& destination) noexcept {
    destination = "PropertyWindow";
    return destination;
}

const char* PropertyWindowRegistration::RegisterNameThunk(
    const char*& destination) noexcept {
    return RegisterName(destination);
}

std::uint32_t PropertyWindowRegistration::CreateObject(
    std::uint32_t name, IPropertyWindowRegistrationRuntime& runtime) {
    const auto storage = runtime.Allocate(0xb8u);
    if (storage == 0u) return 0u;
    try {
        return runtime.Construct(storage, name);
    } catch (...) {
        runtime.Free(storage);
        throw;
    }
}

std::uint32_t PropertyWindowRegistration::RegisterFactory(
    IPropertyWindowRegistrationRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    return runtime.RegisterFactory(manager, runtime.RegisteredName(),
                                   &CreateObject);
}

std::uint32_t PropertyWindowRegistration::RegisterFactoryThunk(
    IPropertyWindowRegistrationRuntime& runtime) {
    return RegisterFactory(runtime);
}

std::uint32_t PropertyWindowRegistration::RegisterShowCommand(
    IPropertyWindowRegistrationRuntime& runtime) {
    const auto name = runtime.LiteralName();
    auto callback = runtime.Allocate(8u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 4u, name);
        runtime.WriteWord(callback, kCommandCallbackVtable);
    }
    try {
        const auto manager = runtime.FrameManager();
        runtime.InsertOrReplace(manager + 0x104u, "show_property_ui",
                                callback);
    } catch (...) {
        if (callback != 0u) runtime.DeleteCallback(callback, 1u);
        throw;
    }
    if (callback != 0u) runtime.DeleteCallback(callback, 1u);
    return kCommandResultAddress;
}

std::uint32_t PropertyWindowRegistration::RegisterShowCommandThunk(
    IPropertyWindowRegistrationRuntime& runtime) {
    return RegisterShowCommand(runtime);
}

}  // namespace pal4
