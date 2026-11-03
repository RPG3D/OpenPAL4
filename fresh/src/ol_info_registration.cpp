#include "pal4/ol_info_registration.h"

namespace pal4 {

const char* OlInfoRegistration::RegisterName(
    const char*& destination) noexcept {
    destination = "olInfo";
    return destination;
}

const char* OlInfoRegistration::RegisterNameThunk(
    const char*& destination) noexcept {
    return RegisterName(destination);
}

std::uint32_t OlInfoRegistration::CreateObject(
    std::uint32_t name, IOlInfoRegistrationRuntime& runtime) {
    const auto storage = runtime.Allocate(0x54u);
    if (storage == 0u) return 0u;
    try {
        return runtime.Construct(storage, name);
    } catch (...) {
        runtime.Free(storage);
        throw;
    }
}

std::uint32_t OlInfoRegistration::RegisterFactory(
    IOlInfoRegistrationRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    const auto name = runtime.RegisteredName();
    return runtime.RegisterFactory(manager, name, &CreateObject);
}

std::uint32_t OlInfoRegistration::RegisterFactoryThunk(
    IOlInfoRegistrationRuntime& runtime) {
    return RegisterFactory(runtime);
}

std::uint32_t OlInfoRegistration::RegisterShowCommand(
    IOlInfoRegistrationRuntime& runtime) {
    const auto name = runtime.WindowNameAddress();
    auto callback = runtime.Allocate(8u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 4u, name);
        runtime.WriteWord(callback, kCommandCallbackVtable);
    }
    try {
        const auto manager = runtime.FrameManager();
        runtime.InsertOrReplace(manager + 0x104u, "show_ol_info_ui",
                                callback);
    } catch (...) {
        if (callback != 0u) runtime.DeleteCallback(callback, 1u);
        throw;
    }
    if (callback != 0u) runtime.DeleteCallback(callback, 1u);
    return kCommandResultAddress;
}

std::uint32_t OlInfoRegistration::RegisterShowCommandThunk(
    IOlInfoRegistrationRuntime& runtime) {
    return RegisterShowCommand(runtime);
}

}  // namespace pal4
