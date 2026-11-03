#include "pal4/main_window_visibility.h"

namespace pal4 {

std::int32_t MainWindowVisibility::SetVisibility(
    MainWindowState& state, std::uint32_t object_address,
    std::uint32_t showing, std::uint32_t parent,
    IMainWindowVisibilityRuntime& runtime) {
    runtime.SetParent(state, showing, parent);
    const auto member = object_address == 0u ? 0u :
                        object_address + 0x28u;
    if ((showing & 0xffu) == 0u) {
        const auto manager = runtime.FrameManager();
        return runtime.RemoveActive(manager + 0x134u, member);
    }

    state.Write(0x34u, state.Read(0x30u));
    for (std::uint32_t offset = 0x40u;
         offset <= 0x58u; offset += 4u)
        runtime.RegisterMouse(member, state.Read(offset));

    const auto global = runtime.GlobalData();
    const auto enabled = runtime.ReadSignedWord(global + 0x28u) > 0;
    runtime.SetVisible(state.Read(0x64u), enabled);
    runtime.SetVisible(state.Read(0x5cu), enabled);
    if (enabled) runtime.RegisterMouse(member, state.Read(0x5cu));

    const auto manager = runtime.FrameManager();
    return runtime.InsertActive(manager + 0x134u, member);
}

}  // namespace pal4
