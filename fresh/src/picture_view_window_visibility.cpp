#include "pal4/picture_view_window_visibility.h"

namespace pal4 {

std::uint32_t PictureViewWindowVisibility::SetVisible(
    PictureViewWindowState& state, std::uint32_t object_address,
    std::uint8_t showing, std::uint32_t parent,
    IPictureViewWindowVisibilityRuntime& runtime) {
    runtime.SetParent(state, showing, parent);
    state.Write(0x3cu, 1u);
    const auto manager = runtime.FrameManager();
    const auto array = manager + 0x134u;
    const auto member = object_address + 0x28u;
    return showing != 0u ? runtime.InsertActive(array, member) :
                           runtime.RemoveActive(array, member);
}

}  // namespace pal4
