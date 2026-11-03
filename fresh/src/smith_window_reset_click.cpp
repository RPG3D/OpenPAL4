#include "pal4/smith_window.h"

namespace pal4 {

std::uint8_t SmithWindow::OnResetClickThunk(
    const std::uint32_t object, const std::uint32_t,
    ISmithWindowResetClickRuntime& runtime) {
    return runtime.OnResetClick(object);
}

}  // namespace pal4
