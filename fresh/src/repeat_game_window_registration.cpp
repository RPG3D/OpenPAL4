#include "pal4/repeat_game_window_registration.h"
#include "pal4/detail/window_registration_core.h"

namespace pal4 {

std::uint32_t RepeatGameWindowRegistration::InitializeNameThunk(
    IRepeatGameWindowRegistrationRuntime& runtime) {
    return InitializeName(runtime);
}

std::uint32_t RepeatGameWindowRegistration::InitializeName(
    IRepeatGameWindowRegistrationRuntime& runtime) {
    return detail::WindowRegistrationCore::InitializeName(
        kNamePointer, 0x8eac90u, runtime);
}

std::uint32_t RepeatGameWindowRegistration::Create(
    const std::uint32_t layout_name,
    IRepeatGameWindowRegistrationRuntime& runtime) {
    return detail::WindowRegistrationCore::Create(layout_name, runtime);
}

std::uint32_t RepeatGameWindowRegistration::RegisterFactoryThunk(
    IRepeatGameWindowRegistrationRuntime& runtime) {
    return RegisterFactory(runtime);
}

std::uint32_t RepeatGameWindowRegistration::RegisterFactory(
    IRepeatGameWindowRegistrationRuntime& runtime) {
    return detail::WindowRegistrationCore::RegisterFactory(
        0x8eac90u, kFactory, runtime);
}

std::uint32_t RepeatGameWindowRegistration::RegisterHandlerThunk(
    IRepeatGameWindowRegistrationRuntime& runtime) {
    return RegisterHandler(runtime);
}

std::uint32_t RepeatGameWindowRegistration::RegisterHandler(
    IRepeatGameWindowRegistrationRuntime& runtime) {
    return detail::WindowRegistrationCore::RegisterHandler(
        kNamePointer, kEventKey, 0x8eac8cu, runtime);
}

}  // namespace pal4
