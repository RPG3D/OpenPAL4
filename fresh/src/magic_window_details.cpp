#include "pal4/magic_window_details.h"

namespace pal4 {

void MagicWindowDetails::UpdateMagic(
    std::int32_t index, const MagicWindowPanelState& panel,
    IMagicWindowDetailsRuntime& runtime) {
    if (index < 0)
        return;

    for (std::uint32_t row = 0; row < 6u; ++row)
        runtime.SetSelected(panel.At(0xc0u + 4u * row), false);
    const auto selected = static_cast<std::uint32_t>(index);
    runtime.SetSelected(panel.At(0xc0u + 4u * selected), true);
    const auto key = runtime.ButtonKey(panel.At(0xf0u + 4u * selected));
    const auto detail = runtime.FindMagicDetail(key);
    if (detail) {
        runtime.SetText(panel.At(0x88u), detail->title);
        runtime.SetText(panel.At(0x8cu), detail->description);
    }
}

void MagicWindowDetails::UpdateStunt(
    std::int32_t index, const MagicWindowPanelState& panel,
    IMagicWindowDetailsRuntime& runtime) {
    if (index < 0)
        return;

    const auto selected = static_cast<std::uint32_t>(index);
    const auto button = panel.At(0x108u + 4u * selected);
    if (!runtime.IsVisible(button))
        return;

    for (std::uint32_t row = 0; row < 7u; ++row)
        runtime.SetSelected(panel.At(0x108u + 4u * row), false);
    runtime.SetSelected(button, true);
    const auto detail = runtime.FindStuntDetail(runtime.ButtonKey(button));
    if (detail) {
        runtime.SetText(panel.At(0x88u), detail->title);
        runtime.SetText(panel.At(0x8cu), detail->description);
    }
}

}  // namespace pal4
