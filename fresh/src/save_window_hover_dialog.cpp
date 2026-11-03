#include "pal4/save_window_hover_dialog.h"

namespace pal4 {

std::uint8_t SaveWindowHoverDialog::Display(
    SaveWindowState& state, std::uint32_t object_address,
    ISaveWindowHoverDialogRuntime& runtime) {
    const auto item = runtime.HoverItem(state.Read(0x54u));
    if (item == 0u) return 1u;

    const auto occupied = runtime.ReadWord(item + 0x1ecu) != 0u;
    const auto callback = runtime.AllocateCallback(0x18u);
    if (callback != 0u) {
        runtime.WriteCallbackWord(callback + 8u, kSaveHandler);
        runtime.WriteCallbackWord(callback + 0xcu, 0u);
        runtime.WriteCallbackWord(callback + 0x10u, object_address);
        runtime.WriteCallbackWord(callback, kCallbackVtable);
    }
    try {
        const auto text = runtime.TriggerItemText(occupied ? 109u : 231u);
        runtime.CreateCommonDialog(runtime.FrameManager());
        runtime.ShowDialog(text, 0.3f, 0.4f, 3u, callback);
    } catch (...) {
        if (callback != 0u) runtime.DeleteCallback(callback, 1u);
        throw;
    }
    if (callback != 0u) runtime.DeleteCallback(callback, 1u);
    return 1u;
}

}  // namespace pal4
