#include "pal4/smith_window.h"

#include <bit>
#include <cstdint>

namespace pal4 {

void SmithWindow::RefreshEquipmentAvailability(
    const std::uint32_t object,
    ISmithWindowEquipmentAvailabilityRuntime& runtime) {
    const auto begin = runtime.ReadWord(object + 0xccu);
    const auto count = begin == 0u
                           ? 0u
                           : static_cast<std::uint32_t>(
                                 std::bit_cast<std::int32_t>(
                                     runtime.ReadWord(object + 0xd0u) - begin) >> 2);
    const bool show_pages = begin != 0u && count > 1u;
    runtime.SetVisible(runtime.ReadWord(object + 0x58u), show_pages);
    runtime.SetVisible(runtime.ReadWord(object + 0x5cu), show_pages);

    if (runtime.ReadWord(object + 0xb4u) == 0u) return;
    const auto list = runtime.ReadWord(object + 0x40u);
    const auto rows = runtime.RowCount(list);
    const auto columns = runtime.ColumnCount(
        runtime.ReadWord(object + 0x40u));
    const auto prescription_id = runtime.PrescriptionHoverItemId(object);
    for (std::int32_t row = 0; row < rows; ++row) {
        const auto first_item = runtime.ItemAt(
            runtime.ReadWord(object + 0x40u), row, 0);
        const auto equipment_id = runtime.ReadWord(first_item + 0x138u);
        const auto mode = runtime.ReadWord(object + 0xb4u);
        bool available = true;
        if (mode == 1u) {
            runtime.RegisterSmithSystem();
            available = runtime.EquipmentProperty(equipment_id) > 0;
        } else if (mode == 2u) {
            runtime.RegisterSmithSystem();
            available = runtime.EquipmentPropertySufficient(
                prescription_id, equipment_id);
        }
        for (std::int32_t column = 0; column < columns; ++column) {
            const auto item = runtime.ItemAt(
                runtime.ReadWord(object + 0x40u), row, column);
            runtime.WriteByte(item + 0x1a5u, available ? 0u : 1u);
        }
    }
}

}  // namespace pal4
