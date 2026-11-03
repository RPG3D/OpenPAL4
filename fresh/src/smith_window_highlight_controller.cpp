#include "pal4/smith_window_highlight_controller.h"

#include <cstdint>

namespace pal4 {

std::uint8_t SmithWindowHighlightController::OnAutoClose(
    const std::uint32_t object,
    ISmithWindowHighlightRuntime& runtime) {
    SetHighlighted(object, 0u, runtime);
    return 1u;
}

void SmithWindowHighlightController::SetHighlighted(
    const std::uint32_t object, const std::uint8_t highlighted,
    ISmithWindowHighlightRuntime& runtime) {
    const auto owner = runtime.ReadWord(object + 4u);
    runtime.WriteByte(object + 8u, highlighted);
    const auto primary = runtime.ReadWord(owner + 0x58u);
    if (!runtime.IsVisible(primary, false)) {
        runtime.WriteByte(object + 8u, 0u);
    }

    if (highlighted != 0u) {
        const auto secondary = runtime.ReadWord(owner + 0x5cu);
        if (runtime.IsVisible(secondary, false)) {
            const auto target = runtime.ReadWord(owner + 0x5cu);
            runtime.WriteByte(target + 0x4b1u, 1u);
            runtime.RequestRedraw(target);
        }
    } else {
        const auto secondary = runtime.ReadWord(owner + 0x5cu);
        runtime.WriteByte(secondary + 0x4b1u, 0u);
        runtime.RequestRedraw(secondary);
        const auto primary_again = runtime.ReadWord(owner + 0x58u);
        runtime.WriteByte(primary_again + 0x4b1u, 0u);
        runtime.RequestRedraw(primary_again);
    }
}

}  // namespace pal4
