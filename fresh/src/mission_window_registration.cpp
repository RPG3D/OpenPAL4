#include "pal4/mission_window_registration.h"

namespace pal4 {

const char* MissionWindowRegistration::RegisterName(
    const char*& destination) noexcept {
    destination = "MissionWindow";
    return destination;
}

const char* MissionWindowRegistration::RegisterNameThunk(
    const char*& destination) noexcept {
    return RegisterName(destination);
}

std::uint32_t MissionWindowRegistration::CreateObject(
    std::uint32_t name, IMissionWindowRegistrationRuntime& runtime) {
    const auto storage = runtime.Allocate(0x8cu);
    if (storage == 0u) return 0u;
    try {
        return runtime.Construct(storage, name);
    } catch (...) {
        runtime.Free(storage);
        throw;
    }
}

std::uint32_t MissionWindowRegistration::RegisterFactory(
    IMissionWindowRegistrationRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    return runtime.RegisterFactory(manager, runtime.RegisteredName(),
                                   &CreateObject);
}

std::uint32_t MissionWindowRegistration::RegisterFactoryThunk(
    IMissionWindowRegistrationRuntime& runtime) {
    return RegisterFactory(runtime);
}

std::uint32_t MissionWindowRegistration::RegisterShowCommand(
    IMissionWindowRegistrationRuntime& runtime) {
    const auto name = runtime.LiteralName();
    auto callback = runtime.Allocate(8u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 4u, name);
        runtime.WriteWord(callback, kCommandCallbackVtable);
    }
    try {
        const auto manager = runtime.FrameManager();
        runtime.InsertOrReplace(manager + 0x104u, "show_mission_ui",
                                callback);
    } catch (...) {
        if (callback != 0u) runtime.DeleteCallback(callback, 1u);
        throw;
    }
    if (callback != 0u) runtime.DeleteCallback(callback, 1u);
    return kCommandResultAddress;
}

std::uint32_t MissionWindowRegistration::RegisterShowCommandThunk(
    IMissionWindowRegistrationRuntime& runtime) {
    return RegisterShowCommand(runtime);
}

}  // namespace pal4
