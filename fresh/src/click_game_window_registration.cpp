#include "pal4/click_game_window_registration.h"
#include "pal4/detail/window_registration_core.h"

namespace pal4 {

std::uint32_t ClickGameWindowRegistration::InitializeNameThunk(
    IClickGameWindowRegistrationRuntime& runtime) {
    return InitializeName(runtime);
}

std::uint32_t ClickGameWindowRegistration::InitializeName(
    IClickGameWindowRegistrationRuntime& runtime) {
    return detail::WindowRegistrationCore::InitializeName(
        0x8b1208u, 0x8eac78u, runtime);
}

std::uint32_t ClickGameWindowRegistration::Create(
    const std::uint32_t layout_name,
    IClickGameWindowRegistrationRuntime& runtime) {
    return detail::WindowRegistrationCore::Create(layout_name, runtime);
}

std::uint32_t ClickGameWindowRegistration::RegisterFactoryThunk(
    IClickGameWindowRegistrationRuntime& runtime) {
    return RegisterFactory(runtime);
}

std::uint32_t ClickGameWindowRegistration::RegisterFactory(
    IClickGameWindowRegistrationRuntime& runtime) {
    return detail::WindowRegistrationCore::RegisterFactory(
        0x8eac78u, 0x49f270u, runtime);
}

std::uint32_t ClickGameWindowRegistration::RegisterHandlerThunk(
    IClickGameWindowRegistrationRuntime& runtime) {
    return RegisterHandler(runtime);
}

std::uint32_t ClickGameWindowRegistration::RegisterHandler(
    IClickGameWindowRegistrationRuntime& runtime) {
    return detail::WindowRegistrationCore::RegisterHandler(
        0x8b1208u, 0x8b1204u, 0x8eac74u, runtime);
}

}  // namespace pal4
