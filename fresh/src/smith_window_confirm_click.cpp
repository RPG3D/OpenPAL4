#include "pal4/smith_window.h"

namespace pal4 {

std::uint8_t SmithWindow::OnConfirmClickThunk(
    const std::uint32_t object, const std::uint32_t,
    ISmithWindowConfirmClickRuntime& runtime) {
    return runtime.OnConfirmTrade(object);
}

}  // namespace pal4
