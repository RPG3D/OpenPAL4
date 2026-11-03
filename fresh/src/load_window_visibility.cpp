#include "pal4/load_window_visibility.h"

namespace pal4 {

std::int32_t LoadWindowVisibility::SetVisibility(
    LoadWindowState& state, std::uint32_t object_address,
    std::uint32_t showing, std::uint32_t parent,
    ILoadWindowVisibilityRuntime& runtime) {
    runtime.SetParent(state, showing, parent);
    const auto member = object_address == 0u ? 0u :
                        object_address + 0x28u;
    if ((showing & 0xffu) != 0u) {
        const auto global_data = runtime.GlobalData();
        const auto save_count = static_cast<std::int32_t>(
            runtime.ReadWord(global_data + 0x2cu));
        runtime.PopulateSaveList(state, save_count / 10, 1u);
        const auto manager = runtime.FrameManager();
        return runtime.InsertActive(manager + 0x134u, member);
    }
    const auto manager = runtime.FrameManager();
    return runtime.RemoveActive(manager + 0x134u, member);
}

}  // namespace pal4
