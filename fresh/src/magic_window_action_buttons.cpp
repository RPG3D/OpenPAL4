#include "pal4/magic_window_action_buttons.h"

namespace pal4 {

void MagicWindowActionButtons::Reset(
    const MagicWindowPanelState& panel,
    IMagicWindowActionButtonRuntime& runtime) {
    for (std::uint32_t index = 0; index < 6u; ++index) {
        const auto button = panel.At(0xf0u + 4u * index);
        runtime.SetVisible(button, false);
        runtime.SetEnabled(button, true);
        runtime.SetKey(button, 0u);
    }
}

}  // namespace pal4
