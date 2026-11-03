#include "pal4/smith_window.h"

namespace pal4 {
namespace {

std::int32_t HoverId(const std::uint32_t object,
                     const std::uint32_t list_offset,
                     ISmithWindowHoverRuntime& runtime) {
    const auto list = runtime.ReadWord(object + list_offset);
    const auto item = runtime.HoverItem(list);
    if (item == 0u) return -1;
    return static_cast<std::int32_t>(
        runtime.ReadWord(item + 0x138u));
}

}  // namespace

std::int32_t SmithWindow::GetEquipmentHoverItemId(
    const std::uint32_t object,
    ISmithWindowHoverRuntime& runtime) {
    return HoverId(object, 0x40u, runtime);
}

std::int32_t SmithWindow::GetPrescriptionHoverItemId(
    const std::uint32_t object,
    ISmithWindowHoverRuntime& runtime) {
    return HoverId(object, 0x44u, runtime);
}

std::uint8_t SmithWindow::OnEquipmentListClick(
    const std::uint32_t object, const std::uint32_t,
    ISmithWindowEquipmentClickRuntime& runtime) {
    const auto id = GetEquipmentHoverItemId(object, runtime);
    const auto selected = runtime.FindInventoryItem(
        static_cast<std::uint32_t>(id), 7u);
    UpdateEquipmentItemName(object, selected, runtime);
    UpdateEquipmentItemDescription(object, selected, runtime);
    return 1u;
}

}  // namespace pal4
