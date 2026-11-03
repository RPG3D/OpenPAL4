#include "pal4/magic_window_selection.h"

namespace pal4 {

std::int32_t MagicWindowSelection::SelectedMagicRow(
    const MagicWindowPanelState& panel,
    IMagicWindowSelectionRuntime& runtime) {
    for (std::uint32_t row = 0; row < 6u; ++row) {
        if (runtime.IsSelected(panel.At(0xc0u + 4u * row)))
            return static_cast<std::int32_t>(row);
    }
    return -1;
}

std::int32_t MagicWindowSelection::SelectedStuntRow(
    const MagicWindowPanelState& panel,
    IMagicWindowSelectionRuntime& runtime) {
    for (std::uint32_t row = 0; row < 7u; ++row) {
        if (runtime.IsSelected(panel.At(0x108u + 4u * row)))
            return static_cast<std::int32_t>(row);
    }
    return -1;
}

}  // namespace pal4
