#include "pal4/smith_window.h"

#include <cstdint>

namespace pal4 {

std::int32_t SmithWindow::RefreshPrescriptionAvailability(
    const std::uint32_t object,
    ISmithWindowPrescriptionAvailabilityRuntime& runtime) {
    const auto list = runtime.ReadWord(object + 0x44u);
    const auto rows = runtime.RowCount(list);
    const auto columns = runtime.ColumnCount(
        runtime.ReadWord(object + 0x44u));
    std::int32_t result = 0;
    for (std::int32_t row = 0; row < rows; ++row) {
        const auto first_item = runtime.ItemAt(
            runtime.ReadWord(object + 0x44u), row, 0);
        const auto recipe_id = runtime.ReadWord(first_item + 0x138u);
        bool available = true;
        if (runtime.ReadWord(object + 0xb4u) <= 2u) {
            runtime.RegisterSmithSystem();
            available = runtime.CraftingRequirementsMet(recipe_id);
        }
        for (std::int32_t column = 0; column < columns; ++column) {
            const auto item = runtime.ItemAt(
                runtime.ReadWord(object + 0x44u), row, column);
            runtime.WriteByte(item + 0x1a5u, available ? 0u : 1u);
        }
        result = row + 1;
    }
    return result;
}

}  // namespace pal4
