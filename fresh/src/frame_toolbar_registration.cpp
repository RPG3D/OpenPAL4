#include "pal4/frame_toolbar_registration.h"
#include "pal4/detail/window_registration_core.h"

namespace pal4 {

std::uint32_t FrameToolbarRegistration::InitializeNameThunk(
    IFrameToolbarRegistrationRuntime& runtime) {
    return InitializeName(runtime);
}

std::uint32_t FrameToolbarRegistration::InitializeName(
    IFrameToolbarRegistrationRuntime& runtime) {
    return detail::WindowRegistrationCore::InitializeName(
        kNamePointer, 0x8eacb4u, runtime);
}

std::uint32_t FrameToolbarRegistration::Create(
    const std::uint32_t layout_name,
    IFrameToolbarRegistrationRuntime& runtime) {
    return detail::WindowRegistrationCore::Create(layout_name, 0x54u,
                                                   runtime);
}

std::uint32_t FrameToolbarRegistration::RegisterFactoryThunk(
    IFrameToolbarRegistrationRuntime& runtime) {
    return RegisterFactory(runtime);
}

std::uint32_t FrameToolbarRegistration::RegisterFactory(
    IFrameToolbarRegistrationRuntime& runtime) {
    return detail::WindowRegistrationCore::RegisterFactory(
        0x8eacb4u, kFactory, runtime);
}

std::uint32_t FrameToolbarRegistration::RegisterHandlerThunk(
    IFrameToolbarRegistrationRuntime& runtime) {
    return RegisterHandler(runtime);
}

std::uint32_t FrameToolbarRegistration::RegisterHandler(
    IFrameToolbarRegistrationRuntime& runtime) {
    return detail::WindowRegistrationCore::RegisterHandler(
        kNamePointer, kEventKey, 0x8eacb0u, runtime);
}

}  // namespace pal4
