#include "pal4/smith_window.h"

#include <cstdint>

namespace pal4 {

std::uint8_t SmithWindow::OnCloseConfirm(
    const std::uint32_t object, const std::uint32_t event,
    ISmithWindowCloseConfirmRuntime& runtime) {
    if (runtime.ReadWord(event + 0x10u) != 1u) {
        return runtime.HandleBaseKeyEvent(object, event);
    }

    const bool active = runtime.SmithActive();
    runtime.ClearModelPreview(object - 0x28u);
    if (active) {
        runtime.ProcessUIEvent(0x8b0f1cu, 0u, 0u);
        runtime.ProcessUIEvent(0x8a80acu, 1u, 0u);
    } else {
        runtime.ProcessUIEvent(0x8a80b4u, 0u, 0u);
    }
    return 1u;
}

}  // namespace pal4
