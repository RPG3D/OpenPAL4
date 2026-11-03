#include "pal4/smith_window.h"

#include <array>
#include <bit>
#include <cstdint>
#include <string>

namespace pal4 {
namespace {

const std::array<float, 2> kDialogSize{
    std::bit_cast<float>(0x3e99999au),
    std::bit_cast<float>(0x3e99999au),
};

}  // namespace

std::uint8_t SmithWindow::OnExtendedCraftConfirm(
    const std::uint32_t object, const std::uint32_t event,
    ISmithWindowExtendedCraftRuntime& runtime) {
    if (event != 1u) return 1u;

    const auto prescription_id = runtime.PrescriptionHoverItemId(object);
    const auto equipment_id = runtime.EquipmentHoverItemId(object);
    runtime.RegisterSmithSystem();
    if (runtime.ProcessExtendedCraft(prescription_id, equipment_id) == 0) {
        runtime.CreateCommonDialog(runtime.UIFrameManager());
        runtime.RegisterSmithSystem();
        const auto current_id = runtime.EquipmentHoverItemId(object);
        const auto name = runtime.ItemDataName(current_id);
        const auto message = runtime.FormatMessage(
            runtime.DialogTemplate(0x3fu), name);
        auto callback = runtime.CreateCloseCallback(object);
        try {
            runtime.ShowDialog(message, kDialogSize, 4u, callback);
            const auto selected_id = runtime.EquipmentHoverItemId(object);
            const auto selected = runtime.FindInventoryItem(
                static_cast<std::uint32_t>(selected_id), 7u);
            UpdateEquipmentItemName(object, selected, runtime);
            UpdateEquipmentItemDescription(object, selected, runtime);
        } catch (...) {
            if (callback != 0u) runtime.DestroyCallback(callback, 1u);
            throw;
        }
        if (callback != 0u) runtime.DestroyCallback(callback, 1u);
    }
    runtime.Call49F080(object);
    runtime.Call49EF80(object);
    runtime.HideSelectionPanel(object);
    return 1u;
}

std::uint8_t SmithWindow::OnPageChangeConfirm(
    const std::uint32_t object, const std::uint32_t event,
    ISmithWindowExtendedCraftRuntime& runtime) {
    return OnExtendedCraftConfirm(object, event, runtime);
}

}  // namespace pal4
