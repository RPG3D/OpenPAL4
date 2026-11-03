#include "pal4/save_window_visibility.h"

namespace pal4 {

std::uint32_t SaveWindowVisibility::SetVisible(
    SaveWindowState& state, std::uint32_t object_address,
    std::uint8_t showing, std::uint32_t parent,
    ISaveWindowVisibilityRuntime& runtime) {
    runtime.SetParent(state, showing, parent);
    if (showing != 0u) {
        runtime.PopulateSaveList(state, runtime.GlobalSaveIndex() / 10, true);
        const auto current_state = runtime.CurrentState();
        if (current_state != 0u && runtime.StateId(current_state) != 2u)
            runtime.CaptureActiveCameraRaster(runtime.FrameManager());
    } else {
        const auto current_state = runtime.CurrentState();
        if (current_state != 0u && runtime.StateId(current_state) == 2u)
            runtime.NotifyPropertyReturn(runtime.FrameManager() + 0x104u,
                                       0u, 0u);
    }
    runtime.SetAlwaysOnTop(state.Read(0x24u), true);
    const auto manager = runtime.FrameManager();
    if (showing != 0u)
        return runtime.InsertActive(manager + 0x134u, object_address + 0x28u);
    return runtime.RemoveActive(manager + 0x134u, object_address + 0x28u);
}

}  // namespace pal4
