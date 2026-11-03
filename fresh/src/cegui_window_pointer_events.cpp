#include "pal4/cegui_window_pointer_events.h"

namespace pal4 {

CeguiPoint CeguiWindowPointerEvents::AbsolutePosition(
    const std::uint32_t window, ICeguiWindowPointerRuntime& runtime) {
    const auto parent_size = runtime.ParentSize(window);
    const auto offset = runtime.PixelAlignOffset();
    const auto x = static_cast<float>(runtime.FloatToInt(
                       static_cast<double>(parent_size.x *
                                           runtime.ReadFloat(window + 236u)) +
                       offset)) + runtime.ReadFloat(window + 240u);
    const auto y = static_cast<float>(runtime.FloatToInt(
                       static_cast<double>(parent_size.y *
                                           runtime.ReadFloat(window + 244u)) +
                       offset)) + runtime.ReadFloat(window + 248u);
    const auto parent = runtime.ReadWord(window + 68u);
    if (parent != 0u) {
        const auto parent_position = AbsolutePosition(parent, runtime);
        return {x + parent_position.x, y + parent_position.y};
    }
    return {x, y};
}

std::uint32_t CeguiWindowPointerEvents::HandleMouseAtCenter(
    const std::uint32_t window, ICeguiWindowPointerRuntime& runtime) {
    const auto origin = AbsolutePosition(window, runtime);
    const auto width = runtime.ReadFloat(window + 268u);
    const auto height = runtime.ReadFloat(window + 272u);
    CeguiMouseEvent event{};
    event.vtable = runtime.MouseEventVtable();
    event.window = window;
    event.position = {origin.x + width * 0.5f,
                      origin.y + height * 0.5f};
    (void)runtime.InvokeWindowEvent(window, 180u, event);
    return runtime.InvokeWindowEvent(window, 184u, event);
}

}  // namespace pal4
