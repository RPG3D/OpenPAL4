#include "pal4/smith_window.h"

namespace pal4 {

void SmithWindow::UpdatePrescriptionItemDescription(
    const std::uint32_t object, const std::uint32_t recipe,
    ISmithWindowPrescriptionDescriptionRuntime& runtime) {
    if (recipe == 0u) {
        runtime.SetText(runtime.ReadWord(object + 0x54u), {});
        return;
    }

    if (runtime.ReadWord(object + 0xb4u) != 0u) {
        const auto text = runtime.Utf8Text(
            runtime.ReadWord(recipe + 0x28u));
        runtime.SetText(runtime.ReadWord(object + 0x54u), text);
        return;
    }

    const auto equipment = runtime.EquipmentForId(
        runtime.ReadWord(recipe + 0x68u));
    const auto text = runtime.Utf8Buffer(
        runtime.DescribeEquipment(equipment));
    runtime.SetText(runtime.ReadWord(object + 0x54u), text);
}

}  // namespace pal4
