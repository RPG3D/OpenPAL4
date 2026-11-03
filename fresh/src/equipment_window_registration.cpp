#include "pal4/equipment_window_registration.h"

namespace pal4 {

const char* EquipmentWindowRegistration::RegisterName(
    const char*& destination) noexcept {
    destination = "EquipmentWindow";
    return destination;
}

const char* EquipmentWindowRegistration::RegisterNameThunk(
    const char*& destination) noexcept {
    return RegisterName(destination);
}

std::uint32_t EquipmentWindowRegistration::CreateObject(
    std::uint32_t name, IEquipmentWindowRegistrationRuntime& runtime) {
    const auto storage = runtime.Allocate(0x134u);
    if (storage == 0u) return 0u;
    try {
        return runtime.Construct(storage, name);
    } catch (...) {
        runtime.Free(storage);
        throw;
    }
}

std::uint32_t EquipmentWindowRegistration::RegisterFactory(
    IEquipmentWindowRegistrationRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    return runtime.RegisterFactory(manager, runtime.RegisteredName(),
                                   &CreateObject);
}

std::uint32_t EquipmentWindowRegistration::RegisterFactoryThunk(
    IEquipmentWindowRegistrationRuntime& runtime) {
    return RegisterFactory(runtime);
}

std::uint32_t EquipmentWindowRegistration::RegisterShowCommand(
    IEquipmentWindowRegistrationRuntime& runtime) {
    const auto name = runtime.LiteralName();
    auto callback = runtime.Allocate(8u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 4u, name);
        runtime.WriteWord(callback, kCommandCallbackVtable);
    }
    try {
        const auto manager = runtime.FrameManager();
        runtime.InsertOrReplace(manager + 0x104u, "show_equipment_ui",
                                callback);
    } catch (...) {
        if (callback != 0u) runtime.DeleteCallback(callback, 1u);
        throw;
    }
    if (callback != 0u) runtime.DeleteCallback(callback, 1u);
    return kCommandResultAddress;
}

std::uint32_t EquipmentWindowRegistration::RegisterShowCommandThunk(
    IEquipmentWindowRegistrationRuntime& runtime) {
    return RegisterShowCommand(runtime);
}

}  // namespace pal4
