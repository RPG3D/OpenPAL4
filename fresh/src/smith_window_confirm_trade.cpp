#include "pal4/smith_window.h"
#include "smith_window_trade_dialog.h"

#include <cstdint>
#include <string>

namespace pal4 {

std::uint8_t SmithWindow::OnConfirmTrade(
    const std::uint32_t object,
    ISmithWindowConfirmTradeRuntime& runtime) {
    runtime.WriteByte(object + 0xacu, 0u);
    const auto prescription_id = runtime.PrescriptionHoverItemId(object);
    if (prescription_id == -1) return 1u;

    if (!runtime.SmithActive() && runtime.InventoryCount() < 80) {
        ShowSmithTradeDialog(runtime, object, 0x2du, 4u);
        return 1u;
    }

    const auto mode = runtime.ReadWord(object + 0xb4u);
    if (!runtime.CraftingRequirementsMet(prescription_id)) {
        const std::string name(runtime.TradeDataName(prescription_id));
        ShowSmithTradeDialog(runtime, object, 0x31u, 4u, name);
        return 1u;
    }

    if (mode != 0u) {
        runtime.UpdateAvailableEquipmentCategories(object, prescription_id);
        runtime.ShowSelectionPanel(object);
    } else if (runtime.HasElements(prescription_id)) {
        runtime.CollectMatchingInventoryItemIds(object, prescription_id);
        runtime.ShowSelectionPanel(object);
        runtime.RefreshCollectedEquipmentList(object);
    } else {
        if (runtime.SmithActive()) {
            const std::string name(runtime.ElementPropertyName(
                runtime.PrescriptionHoverItemId(object)));
            ShowSmithTradeDialog(runtime, object, 0x32u, 3u, name,
                                 SmithWindowTradeCallback::SelectionConfirm);
        } else {
            ShowSmithTradeDialog(runtime, object, 0x5bu, 3u, {},
                                 SmithWindowTradeCallback::SelectionConfirm);
        }
        return 1u;
    }

    runtime.WriteByte(object + 0xacu, 1u);
    return 1u;
}

}  // namespace pal4
