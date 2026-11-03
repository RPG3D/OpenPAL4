#include "pal4/smith_window.h"

namespace pal4 {

std::uint8_t SmithWindow::OnPrescriptionListClick(
    const std::uint32_t object, const std::uint32_t,
    ISmithWindowPrescriptionClickRuntime& runtime) {
    runtime.ClearEquipmentSelection(object);
    runtime.HideSelectionPanel(object);

    const auto list = runtime.ReadWord(object + 0x44u);
    const auto hover = runtime.HoverItem(list);
    std::uint32_t recipe = 0u;
    if (hover != 0u) {
        const auto id = runtime.ReadWord(hover + 0x138u);
        if (runtime.HasRecipe(id)) recipe = runtime.RecipeForId(id);
    }
    UpdatePrescriptionItemName(object, recipe, runtime);
    UpdatePrescriptionItemDescription(object, recipe, runtime);
    return 1u;
}

}  // namespace pal4
