#include "pal4/smith_window.h"

#include <array>

namespace pal4 {

void SmithWindow::ResetState(const std::uint32_t object,
                             ISmithWindowResetRuntime& runtime) {
    runtime.ClearSelection(object);
    runtime.LoadData(object, 0u);
    const auto list = runtime.ReadWord(object + 0x44u);
    if (runtime.RowCount(list) != 0u)
        runtime.SetHoverRow(list, 0u);

    constexpr std::array<std::uint32_t, 3> buttons{
        0x60u, 0x64u, 0x68u};
    for (const auto offset : buttons)
        runtime.SetSelected(runtime.ReadWord(object + offset), false);
}

}  // namespace pal4
