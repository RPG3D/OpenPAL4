#include "pal4/magic_window_panel_visibility.h"

namespace pal4 {

void MagicWindowPanelVisibility::Set(
    std::uint32_t object, bool visible,
    const MagicWindowPanelState& panel,
    IMagicWindowPanelVisibilityRuntime& runtime) {
    runtime.SetVisible(panel.At(0x15cu), visible);
    runtime.SetVisible(panel.At(0x164u), !visible);
    for (std::uint32_t row = 0; row < 6u; ++row) {
        runtime.SetVisible(panel.At(0xc0u + 4u * row), visible);
        runtime.SetVisible(panel.At(0xf0u + 4u * row), visible);
    }
    runtime.UpdateMagicDetails(object, -1);
}

void MagicWindowPanelVisibility::SetStunt(
    bool visible, const MagicWindowPanelState& panel,
    IMagicWindowPanelVisibilityRuntime& runtime) {
    runtime.SetImage(panel.At(0x154u), "zi0", "xianshuteji");
    for (const auto offset : {0x160u, 0x158u, 0x168u, 0x16cu})
        runtime.SetVisible(panel.At(offset), visible);
}

}  // namespace pal4
