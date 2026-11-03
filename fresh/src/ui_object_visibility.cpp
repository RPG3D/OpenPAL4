#include "pal4/ui_object_visibility.h"

namespace pal4 {

void UiObjectVisibility::SetWindowVisibility(
    UiObjectState& state, bool visible, IUiObjectVisibilityRuntime& runtime) {
    runtime.SetVisible(state.Read(0x50u), visible);
    if (!visible) {
        runtime.SetEnabled(state.Read(0x48u), true);
        runtime.SetEnabled(state.Read(0x4cu), true);
    }
}

int UiObjectVisibility::SetVisibility(
    UiObjectState& state, std::uint32_t object_address,
    std::uint8_t showing, std::uint32_t parent,
    IUiObjectVisibilityRuntime& runtime) {
    runtime.SetParent(state, showing, parent);
    SetWindowVisibility(state, false, runtime);
    const auto manager = runtime.FrameManager();
    const auto active = manager + 0x134u;
    const auto member = object_address + 0x28u;
    if (showing != 0u) {
        runtime.InsertActive(active, member);
        return runtime.SetTextAndVisibility(state, state.Read(0x48u));
    }
    const auto result = runtime.RemoveActive(active, member);
    state.Write(0x3cu, 0u);
    return result;
}

}  // namespace pal4
