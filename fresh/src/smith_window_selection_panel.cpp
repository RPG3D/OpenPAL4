#include "pal4/smith_window.h"

namespace pal4 {

std::uint8_t SmithWindow::ShowSelectionPanel(
    const std::uint32_t object,
    ISmithWindowSelectionPanelRuntime& runtime) {
    const auto shown = runtime.ReadByte(object + 0x81u);
    if (shown != 0u) return shown;

    runtime.SetVisible(runtime.ReadWord(object + 0x90u), true);
    runtime.SetVisible(runtime.ReadWord(object + 0x94u), true);
    runtime.SetVisible(runtime.ReadWord(object + 0x98u), true);
    const auto panel = runtime.ReadWord(object + 0x7cu);
    runtime.WriteByte(object + 0x81u, 1u);
    runtime.SetVisible(panel, true);
    runtime.SetAlwaysOnTop(runtime.ReadWord(object + 0x44u), false);
    runtime.HandleMouseEvent(panel);
    return runtime.InsertFrameRecord(
        runtime.UIFrameManager() + 0x134u, object + 0xf0u);
}

void SmithWindow::HideSelectionPanel(
    const std::uint32_t object,
    ISmithWindowSelectionPanelRuntime& runtime) {
    if (runtime.ReadByte(object + 0x81u) == 0u) return;

    runtime.SetVisible(runtime.ReadWord(object + 0x90u), false);
    runtime.SetVisible(runtime.ReadWord(object + 0x94u), false);
    runtime.SetVisible(runtime.ReadWord(object + 0x98u), false);
    runtime.SetVisible(runtime.ReadWord(object + 0x7cu), false);
    runtime.SetAlwaysOnTop(runtime.ReadWord(object + 0x44u), true);
    runtime.WriteByte(object + 0x81u, 0u);
}

}  // namespace pal4
