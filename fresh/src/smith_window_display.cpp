#include "pal4/smith_window.h"

#include <array>

namespace pal4 {

std::uint32_t SmithWindow::UpdateDisplay(
    const std::uint32_t object,
    ISmithWindowDisplayRuntime& runtime) {
    runtime.WriteByte(object + 0x81u, 0u);
    runtime.WriteByte(object + 0x80u, 0u);

    constexpr std::array<std::uint32_t, 6> panels{
        0x90u, 0x94u, 0x98u, 0x84u, 0x88u, 0x8cu};
    for (const auto offset : panels)
        runtime.SetVisible(runtime.ReadWord(object + offset), false);

    runtime.SetImage(runtime.ReadWord(object + 0x78u),
                     "zi0", "zhuzao");
    runtime.HandleEffect(object + 0xa4u, 1u);
    return runtime.UpdateModelPreview(object, 0u, 0u, 0u);
}

}  // namespace pal4
