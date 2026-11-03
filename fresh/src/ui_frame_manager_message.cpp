#include "pal4/ui_frame_manager_singleton.h"

namespace pal4 {

std::uint8_t UiFrameManagerSingleton::HandleMessage(
    const std::uint32_t manager, const std::uint32_t hwnd,
    const std::uint32_t message, const std::uint32_t wparam,
    const std::uint32_t lparam,
    IUiFrameManagerMessageRuntime& runtime) {
    const auto system = runtime.ReadWord(manager + 0x198u);
    runtime.WriteByte(manager + 0x220u, 0u);
    if (system != 0u) {
        runtime.WriteByte(manager + 0x220u,
                          runtime.ProcessUiEvent(system, hwnd, message,
                                                 wparam, lparam));
    }
    if (message == 0x205u &&
        runtime.ReadWord(manager + 0x198u) != 0u &&
        runtime.ReadByte(manager + 0x221u) != 0u) {
        runtime.SimulateKeyPressAndRelease(manager, 0x1bu);
    }
    return runtime.ReadByte(manager + 0x220u);
}

}  // namespace pal4
