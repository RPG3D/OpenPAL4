#include "pal4/combat_ending_window_registration.h"
#include "pal4/detail/window_registration_core.h"

namespace pal4 {

std::uint32_t CombatEndingWindowRegistration::InitializeNameThunk(
    ICombatEndingWindowRegistrationRuntime& runtime) {
    return InitializeName(runtime);
}

std::uint32_t CombatEndingWindowRegistration::InitializeName(
    ICombatEndingWindowRegistrationRuntime& runtime) {
    return detail::WindowRegistrationCore::InitializeName(
        kNameSource, kNameSlot, runtime);
}

std::uint32_t CombatEndingWindowRegistration::RegisterEventThunk(
    IWindowRegistrationRuntime& runtime) {
    return RegisterEvent(runtime);
}

std::uint32_t CombatEndingWindowRegistration::RegisterEvent(
    IWindowRegistrationRuntime& runtime) {
    return detail::WindowRegistrationCore::RegisterHandler(
        kNameSource, kEventKey, kEventRegistrationSlot, runtime);
}

std::uint32_t CombatEndingWindowFactory::Create(
    const std::uint32_t name, ICombatEndingWindowFactoryRuntime& runtime) {
    const auto object = runtime.Allocate(kObjectSize);
    if (object == 0u) return 0u;
    try {
        return runtime.Construct(object, name);
    } catch (...) {
        runtime.Free(object);
        throw;
    }
}

std::uint32_t CombatEndingWindowFactoryRegistration::RegisterThunk(
    ICombatEndingWindowFactoryRegistrationRuntime& runtime) {
    return Register(runtime);
}

std::uint32_t CombatEndingWindowFactoryRegistration::Register(
    ICombatEndingWindowFactoryRegistrationRuntime& runtime) {
    (void)runtime.UiFrameManagerInstance();
    return UiFrameFactoryTree::InsertRegistration(
        runtime.ReadWord(kNameSlot), kCreateCallback,
        runtime.FactoryGuard(), runtime.FactorySingleton(), runtime);
}

}  // namespace pal4
