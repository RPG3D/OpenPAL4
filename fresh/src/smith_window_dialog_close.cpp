#include "pal4/smith_window.h"

namespace pal4 {

std::uint8_t SmithWindow::OnDialogClose(
    const std::uint32_t object, const std::uint32_t,
    ISmithWindowDialogCloseRuntime& runtime) {
    runtime.InsertFrameRecord(
        runtime.UIFrameManager() + 0x134u, object + 0xe8u);
    return 1u;
}

}  // namespace pal4
