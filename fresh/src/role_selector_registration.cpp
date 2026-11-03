#include "pal4/role_selector_registration.h"

namespace pal4 {

std::uint32_t RoleSelectorRegistration::InitializeNameThunk(
    IRoleSelectorRegistrationRuntime& runtime) {
    return InitializeName(runtime);
}

std::uint32_t RoleSelectorRegistration::InitializeName(
    IRoleSelectorRegistrationRuntime& runtime) {
    const auto name = runtime.ReadWord(0x8b1a5cu);
    runtime.WriteWord(0x8edfa0u, name);
    return name;
}

std::uint32_t RoleSelectorRegistration::Create(
    const std::uint32_t name,
    IRoleSelectorRegistrationRuntime& runtime) {
    const auto object = runtime.Allocate(0x160u);
    if (object == 0u) return 0u;
    try {
        return runtime.Construct(object, name);
    } catch (...) {
        runtime.Free(object);
        throw;
    }
}

std::uint32_t RoleSelectorRegistration::RegisterFactoryThunk(
    IRoleSelectorRegistrationRuntime& runtime) {
    return RegisterFactory(runtime);
}

std::uint32_t RoleSelectorRegistration::RegisterFactory(
    IRoleSelectorRegistrationRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    return runtime.RegisterFactory(
        manager, runtime.ReadWord(0x8edfa0u), 0x4b62d0u);
}

}  // namespace pal4
