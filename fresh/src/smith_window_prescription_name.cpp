#include "pal4/smith_window.h"

namespace pal4 {

void SmithWindow::UpdatePrescriptionItemName(
    const std::uint32_t object, const std::uint32_t recipe,
    ISmithWindowNameRuntime& runtime) {
    const auto text = recipe == 0u
        ? std::string_view{}
        : runtime.Utf8Text(runtime.ReadWord(recipe + 0x18u));
    runtime.SetText(runtime.ReadWord(object + 0x50u), text);
}

}  // namespace pal4
