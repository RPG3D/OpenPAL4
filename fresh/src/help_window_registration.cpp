#include "pal4/help_window_registration.h"
#include "pal4/detail/window_registration_core.h"

namespace pal4 {

std::uint32_t HelpWindowRegistration::InitializeNameThunk(
    IHelpWindowRegistrationRuntime& runtime) {
    return InitializeName(runtime);
}

std::uint32_t HelpWindowRegistration::InitializeName(
    IHelpWindowRegistrationRuntime& runtime) {
    return detail::WindowRegistrationCore::InitializeName(
        kNamePointer, kStoredName, runtime);
}

std::uint32_t HelpWindowRegistration::Create(
    const std::uint32_t layout_name,
    IHelpWindowRegistrationRuntime& runtime) {
    return detail::WindowRegistrationCore::Create(layout_name, 0x54u,
                                                   runtime);
}

std::uint32_t HelpWindowRegistration::RegisterFactoryThunk(
    IHelpWindowRegistrationRuntime& runtime) {
    return RegisterFactory(runtime);
}

std::uint32_t HelpWindowRegistration::RegisterFactory(
    IHelpWindowRegistrationRuntime& runtime) {
    return detail::WindowRegistrationCore::RegisterFactory(
        kStoredName, kFactory, runtime);
}

std::uint32_t HelpWindowRegistration::RegisterShowThunk(
    IHelpWindowRegistrationRuntime& runtime) {
    return RegisterShow(runtime);
}

std::uint32_t HelpWindowRegistration::RegisterShow(
    IHelpWindowRegistrationRuntime& runtime) {
    return detail::WindowRegistrationCore::RegisterCallbackHandler(
        0x4a77a0u, kShowEventKey, 0x8eaca0u, runtime);
}

}  // namespace pal4
