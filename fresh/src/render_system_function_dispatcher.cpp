#include "pal4/render_system_function_dispatcher.h"

namespace pal4 {

std::uint32_t RenderSystemFunctionDispatcher::CallSlot(
    const std::uint32_t offset, const std::uint32_t argument) {
    const std::uint32_t system = runtime_.GetRenderSystem();
    const std::uint32_t function = runtime_.ReadSlot(system, offset);
    return function == 0u ? 2u : runtime_.Invoke(function, argument);
}

std::uint32_t RenderSystemFunctionDispatcher::Call48(
    const std::uint32_t argument) { return CallSlot(0x30u, argument); }
std::uint32_t RenderSystemFunctionDispatcher::Call52(
    const std::uint32_t argument) { return CallSlot(0x34u, argument); }
std::uint32_t RenderSystemFunctionDispatcher::Call56(
    const std::uint32_t argument) { return CallSlot(0x38u, argument); }
std::uint32_t RenderSystemFunctionDispatcher::Call60(
    const std::uint32_t argument) { return CallSlot(0x3cu, argument); }
std::uint32_t RenderSystemFunctionDispatcher::Call64(
    const std::uint32_t argument) { return CallSlot(0x40u, argument); }
std::uint32_t RenderSystemFunctionDispatcher::Call68(
    const std::uint32_t argument) { return CallSlot(0x44u, argument); }
std::uint32_t RenderSystemFunctionDispatcher::Call72(
    const std::uint32_t argument) { return CallSlot(0x48u, argument); }
std::uint32_t RenderSystemFunctionDispatcher::Call76(
    const std::uint32_t argument) { return CallSlot(0x4cu, argument); }
std::uint32_t RenderSystemFunctionDispatcher::Call80(
    const std::uint32_t argument) { return CallSlot(0x50u, argument); }
std::uint32_t RenderSystemFunctionDispatcher::Call84(
    const std::uint32_t argument) { return CallSlot(0x54u, argument); }
std::uint32_t RenderSystemFunctionDispatcher::Call88(
    const std::uint32_t argument) { return CallSlot(0x58u, argument); }
std::uint32_t RenderSystemFunctionDispatcher::Call92(
    const std::uint32_t argument) { return CallSlot(0x5cu, argument); }

std::uint32_t RenderSystemFunctionDispatcher::DispatchKeyboard(
    const std::uint32_t code, const std::uint32_t argument) {
    switch (code) {
    case 22u: return Call48(argument);
    case 23u: return Call52(argument);
    case 24u: return Call56(argument);
    default: return 2u;
    }
}

std::uint32_t RenderSystemFunctionDispatcher::DispatchMouse(
    const std::uint32_t code, const std::uint32_t argument) {
    switch (code) {
    case 5u: return Call60(argument);
    case 6u: return Call64(argument);
    case 7u: return Call68(argument);
    case 8u: return Call72(argument);
    case 9u: return Call76(argument);
    case 10u: return Call80(argument);
    case 11u: return Call88(argument);
    case 12u: return Call84(argument);
    case 13u: return Call92(argument);
    default: return 2u;
    }
}

}  // namespace pal4
