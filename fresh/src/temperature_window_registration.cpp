#include "pal4/temperature_window_registration.h"
#include "pal4/detail/window_registration_core.h"

namespace pal4 {

std::uint32_t TemperatureWindowRegistration::InitializeNameThunk(
    ITemperatureWindowRegistrationRuntime& runtime) {
    return InitializeName(runtime);
}

std::uint32_t TemperatureWindowRegistration::InitializeName(
    ITemperatureWindowRegistrationRuntime& runtime) {
    return detail::WindowRegistrationCore::InitializeName(
        kNamePointer, 0x8eac98u, runtime);
}

std::uint32_t TemperatureWindowRegistration::Create(
    const std::uint32_t layout_name,
    ITemperatureWindowRegistrationRuntime& runtime) {
    return detail::WindowRegistrationCore::Create(layout_name, runtime);
}

std::uint32_t TemperatureWindowRegistration::RegisterFactoryThunk(
    ITemperatureWindowRegistrationRuntime& runtime) {
    return RegisterFactory(runtime);
}

std::uint32_t TemperatureWindowRegistration::RegisterFactory(
    ITemperatureWindowRegistrationRuntime& runtime) {
    return detail::WindowRegistrationCore::RegisterFactory(
        0x8eac98u, kFactory, runtime);
}

std::uint32_t TemperatureWindowRegistration::RegisterHandlerThunk(
    ITemperatureWindowRegistrationRuntime& runtime) {
    return RegisterHandler(runtime);
}

std::uint32_t TemperatureWindowRegistration::RegisterHandler(
    ITemperatureWindowRegistrationRuntime& runtime) {
    return detail::WindowRegistrationCore::RegisterHandler(
        kNamePointer, kEventKey, 0x8eac94u, runtime);
}

}  // namespace pal4
