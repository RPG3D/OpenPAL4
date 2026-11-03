#include "pal4/introduction_window_visibility.h"

namespace pal4 {

void IntroductionWindowVisibility::SetVisibility(
    IntroductionWindowState& state, std::uint32_t object_address,
    std::uint32_t showing, std::uint32_t parent,
    IIntroductionWindowVisibilityRuntime& runtime) {
    runtime.SetParent(state, showing, parent);
    const auto member = object_address == 0u ? 0u : object_address + 0x28u;
    if ((showing & 0xffu) != 0u) {
        SetPanelVisibility(state, false, runtime);
        const auto manager = runtime.FrameManager();
        runtime.InsertActive(manager + 0x134u, member);
        runtime.SetPanelVisibilityAndText(state, state.Read(0x88u));
        runtime.SetPanelTextAndVisibility(state, state.Read(0x98u));
    } else {
        const auto manager = runtime.FrameManager();
        runtime.RemoveActive(manager + 0x134u, member);
        state.Write(0x3cu, 0u);
    }
}

void IntroductionWindowVisibility::SetPanelVisibility(
    IntroductionWindowState& state, bool visible,
    IIntroductionWindowVisibilityRuntime& runtime) {
    runtime.SetVisible(state.Read(0xacu), visible);
    if (!visible) {
        runtime.SetEnabled(state.Read(0x88u), true);
        runtime.SetEnabled(state.Read(0x8cu), true);
        runtime.SetEnabled(state.Read(0x90u), true);
        runtime.SetEnabled(state.Read(0x94u), true);
    }
}

}  // namespace pal4
