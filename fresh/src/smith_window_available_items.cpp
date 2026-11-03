#include "pal4/smith_window.h"

#include <cstdint>

namespace pal4 {

std::uint8_t SmithWindow::UpdateAvailableEquipmentCategories(
    const std::uint32_t object, const std::uint32_t source,
    ISmithWindowAvailableItemsRuntime& runtime) {
    SmithWindowWordArray::Clear(object + 0xc8u, runtime);

    const auto id = runtime.ReadWord(source);
    if (!runtime.HasRecipe(id)) return 0u;

    const auto recipe = runtime.RecipeForId(id);
    for (std::uint32_t category = 0u; category < 7u; ++category) {
        const auto available = static_cast<std::int32_t>(
            runtime.ReadWord(recipe + 0x7cu + category * 4u));
        if (available > 0 && runtime.InventorySize(category) > 0)
            SmithWindowWordArray::Append(object + 0xc8u, category,
                                         runtime);
    }

    runtime.WriteWord(object + 0xb0u, 0xffffffffu);
    const auto current_begin = runtime.ReadWord(object + 0xccu);
    if (current_begin != 0u &&
        runtime.ReadWord(object + 0xd0u) != current_begin) {
        runtime.WriteWord(object + 0xb0u, 0u);
        return static_cast<std::uint8_t>(
            runtime.UpdateEquipmentList(
                object, runtime.ReadWord(current_begin)));
    }
    return static_cast<std::uint8_t>(current_begin);
}

}  // namespace pal4
