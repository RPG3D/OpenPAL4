#include "pal4/game_info_window_registration.h"
#include "pal4/detail/window_registration_core.h"

namespace pal4 {

std::uint32_t GameInfoWindowRegistration::InitializeNameThunk(
    IGameInfoWindowRegistrationRuntime& runtime) {
    return InitializeName(runtime);
}

std::uint32_t GameInfoWindowRegistration::InitializeName(
    IGameInfoWindowRegistrationRuntime& runtime) {
    return detail::WindowRegistrationCore::InitializeName(
        kNamePointer, 0x8eacacu, runtime);
}

std::uint32_t GameInfoWindowRegistration::Create(
    const std::uint32_t layout_name,
    IGameInfoWindowRegistrationRuntime& runtime) {
    return detail::WindowRegistrationCore::Create(layout_name, 0x50u,
                                                   runtime);
}

std::uint32_t GameInfoWindowRegistration::RegisterFactoryThunk(
    IGameInfoWindowRegistrationRuntime& runtime) {
    return RegisterFactory(runtime);
}

std::uint32_t GameInfoWindowRegistration::RegisterFactory(
    IGameInfoWindowRegistrationRuntime& runtime) {
    return detail::WindowRegistrationCore::RegisterFactory(
        0x8eacacu, kFactory, runtime);
}

std::uint32_t GameInfoWindowRegistration::RegisterShowThunk(
    IGameInfoWindowRegistrationRuntime& runtime) {
    return RegisterShow(runtime);
}

std::uint32_t GameInfoWindowRegistration::RegisterShow(
    IGameInfoWindowRegistrationRuntime& runtime) {
    return detail::WindowRegistrationCore::RegisterCallbackHandler(
        0x4a6ba0u, 0x8b13a0u, 0x8eacaau, runtime);
}

std::uint32_t GameInfoWindowRegistration::RegisterHelpThunk(
    IGameInfoWindowRegistrationRuntime& runtime) {
    return RegisterHelp(runtime);
}

std::uint32_t GameInfoWindowRegistration::RegisterHelp(
    IGameInfoWindowRegistrationRuntime& runtime) {
    return detail::WindowRegistrationCore::RegisterCallbackHandler(
        0x4a6bc0u, 0x8b13a4u, 0x8eaca9u, runtime);
}

std::uint32_t GameInfoWindowRegistration::RegisterUpdateThunk(
    IGameInfoWindowRegistrationRuntime& runtime) {
    return RegisterUpdate(runtime);
}

std::uint32_t GameInfoWindowRegistration::RegisterUpdate(
    IGameInfoWindowRegistrationRuntime& runtime) {
    return detail::WindowRegistrationCore::RegisterCallbackHandler(
        0x4a6bf0u, 0x8b13a8u, 0x8eaca8u, runtime);
}

}  // namespace pal4
