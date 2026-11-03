#include "pal4/property_window_visibility.h"

namespace pal4 {

void PropertyWindowVisibility::Set(
    PropertyWindowState& state, std::uint8_t visible,
    std::uint32_t parent, IPropertyWindowVisibilityRuntime& runtime) {
    runtime.SetParent(state, visible, parent);
    runtime.SetItemPanel(state, false);
    runtime.Broadcast(0x8b1394u, visible, 0u);
    runtime.Broadcast(0x8b13a0u, visible, state.Read(36u));
    for (std::uint32_t offset = 64u; offset <= 84u; offset += 4u) {
        runtime.SetSelected(state.Read(offset), false);
    }
    if (visible != 0u) {
        runtime.Broadcast(0x8b13a4u, 6u, 0u);
        runtime.AddActiveMember(state, 40u);
        state.Write(108u, 5u);
        runtime.UpdatePropertyPanel(state);
        runtime.UpdateSystemDialog(2u);
    } else {
        runtime.ToggleModel(state, false);
        runtime.RemoveActiveMember(state, 40u);
    }
}

}  // namespace pal4
