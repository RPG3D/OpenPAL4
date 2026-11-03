#include "pal4/ui_frame_manager_singleton.h"

namespace pal4 {

std::uint8_t UiFrameManagerSingleton::ProcessUiEvents(
    const std::uint32_t manager, const std::uint32_t type,
    const std::uint32_t arguments,
    IUiFrameManagerEventRuntime& runtime) {
    UiFrameEventPayload payload;
    payload.first = runtime.ReadWord(arguments);
    payload.second = runtime.ReadWord(arguments + 4u);
    payload.type = type & 0xffu;
    return runtime.ProcessEvent(manager + 0x104u, 0x8a80a8u, payload);
}

std::uint8_t UiFrameManagerSingleton::SimulateKeyPressAndRelease(
    const std::uint32_t manager, const std::uint32_t key,
    IUiFrameManagerEventRuntime& runtime) {
    (void)runtime.ProcessUiEvent(
        runtime.ReadWord(manager + 0x198u), 0u, 0x100u, key, 0u);
    return runtime.ProcessUiEvent(
        runtime.ReadWord(manager + 0x198u), 0u, 0x101u, key, 0u);
}

}  // namespace pal4
