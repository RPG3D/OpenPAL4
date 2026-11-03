#include "pal4/combat_action_console_window_registration.h"
#include "pal4/detail/window_registration_core.h"

namespace pal4 {

std::uint32_t CombatActionConsoleWindowRegistration::InitializeNameThunk(
    ICombatActionConsoleWindowRegistrationRuntime& runtime) {
    return InitializeName(runtime);
}

std::uint32_t CombatActionConsoleWindowRegistration::InitializeName(
    ICombatActionConsoleWindowRegistrationRuntime& runtime) {
    const auto name = runtime.ReadWord(kNameSource);
    runtime.WriteWord(kNameSlot, name);
    return name;
}

std::uint32_t CombatActionConsoleWindowRegistration::RegisterEventThunk(
    IWindowRegistrationRuntime& runtime) {
    return RegisterEvent(runtime);
}

std::uint32_t CombatActionConsoleWindowRegistration::RegisterEvent(
    IWindowRegistrationRuntime& runtime) {
    return detail::WindowRegistrationCore::RegisterHandler(
        kNameSource, kEventKey, kEventRegistrationSlot, runtime);
}

std::uint32_t CombatActionConsoleWindowFactoryRegistration::RegisterThunk(
    ICombatActionConsoleWindowFactoryRegistrationRuntime& runtime) {
    return Register(runtime);
}

std::uint32_t CombatActionConsoleWindowFactoryRegistration::Register(
    ICombatActionConsoleWindowFactoryRegistrationRuntime& runtime) {
    (void)runtime.UiFrameManagerInstance();
    return UiFrameFactoryTree::InsertRegistration(
        runtime.ReadWord(kNameSlot), kCreateCallback,
        runtime.FactoryGuard(), runtime.FactorySingleton(), runtime);
}

}  // namespace pal4
