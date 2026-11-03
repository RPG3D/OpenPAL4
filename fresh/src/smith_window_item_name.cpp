#include "pal4/smith_window.h"

namespace pal4 {

void SmithWindow::UpdateEquipmentItemName(
    const std::uint32_t object, const std::uint32_t item,
    ISmithWindowNameRuntime& runtime) {
    const auto text = item == 0u
        ? std::string_view{}
        : runtime.Utf8Text(runtime.ReadWord(item + 0xacu));
    runtime.SetText(runtime.ReadWord(object + 0x48u), text);
}

}  // namespace pal4
