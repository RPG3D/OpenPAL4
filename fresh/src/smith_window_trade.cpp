#include "pal4/smith_window.h"
#include "smith_window_trade_dialog.h"

#include <cstdint>
#include <string>

namespace pal4 {

std::uint8_t SmithWindow::OnResetTrade(
    const std::uint32_t object, ISmithWindowTradeRuntime& runtime) {
    if (runtime.ReadByte(object + 0x81u) == 0u ||
        runtime.ReadByte(object + 0xacu) == 0u) {
        return 1u;
    }

    const auto equipment_id = runtime.EquipmentHoverItemId(object);
    if (equipment_id == -1) return 1u;
    const auto prescription_id = runtime.PrescriptionHoverItemId(object);

    if (!runtime.SmithActive() && runtime.InventoryCount() < 80) {
        ShowSmithTradeDialog(runtime, object, 0x2du, 4u);
        return 1u;
    }

    const auto mode = runtime.ReadWord(object + 0xb4u);
    if (mode == 0u) {
        if (!runtime.SmithActive()) {
            ShowSmithTradeDialog(runtime, object, 0x5bu, 3u, {},
                            SmithWindowTradeCallback::SelectionConfirm);
        } else {
            const std::string name(
                runtime.ElementPropertyName(runtime.PrescriptionHoverItemId(object)));
            ShowSmithTradeDialog(runtime, object, 0x32u, 3u, name,
                            SmithWindowTradeCallback::SelectionConfirm);
        }
        return 1u;
    }

    const auto list = runtime.ReadWord(object + 0x40u);
    if (runtime.EquipmentRowCount(list) == 0u) return 1u;

    if (mode == 1u) {
        if (runtime.EquipmentProperty(equipment_id) <= 0) {
            ShowSmithTradeDialog(runtime, object, 0xbeu, 4u);
            return 1u;
        }
        const bool active = runtime.SmithActive();
        const bool below = runtime.QuantityBelowRequirement(
            runtime.PrescriptionHoverItemId(object),
            runtime.EquipmentHoverItemId(object));
        if (!active && !below) {
            ShowSmithTradeDialog(runtime, object, 0x5cu, 3u, {},
                            SmithWindowTradeCallback::EquipmentSelectionConfirm);
        } else if (!active) {
            runtime.AssignInventoryText(object + 0xd8u,
                                        runtime.EquipmentHoverItemId(object));
            const std::string name(
                runtime.InventoryTextName(equipment_id));
            ShowSmithTradeDialog(runtime, object, 0x37u, 3u, name,
                            SmithWindowTradeCallback::EquipmentSelectionPrompt);
        } else if (!below) {
            const std::string name(
                runtime.InventoryDisplayName(runtime.EquipmentHoverItemId(object)));
            ShowSmithTradeDialog(runtime, object, 0x36u, 3u, name,
                            SmithWindowTradeCallback::EquipmentSelectionConfirm);
        } else {
            runtime.AssignInventoryText(object + 0xd8u,
                                        runtime.EquipmentHoverItemId(object));
            const std::string name(
                runtime.InventoryTextName(equipment_id));
            ShowSmithTradeDialog(runtime, object, 0x37u, 3u, name,
                            SmithWindowTradeCallback::ItemUseConfirm);
        }
        return 1u;
    }

    if (!runtime.CraftingRequirementsMet(prescription_id)) {
        const std::string name(runtime.TradeDataName(prescription_id));
        ShowSmithTradeDialog(runtime, object, 0x31u, 4u, name);
        return 1u;
    }
    if (!runtime.EquipmentPropertySufficient(
            runtime.PrescriptionHoverItemId(object),
            runtime.EquipmentHoverItemId(object))) {
        const std::string name(
            runtime.InventoryDisplayName(runtime.EquipmentHoverItemId(object)));
        ShowSmithTradeDialog(runtime, object, 0x3du, 4u, name);
        return 1u;
    }

    const bool active = runtime.SmithActive();
    if (!runtime.EquipmentPropertyNonZero(equipment_id)) {
        if (active) {
            const std::string name(
                runtime.InventoryDisplayName(runtime.EquipmentHoverItemId(object)));
            ShowSmithTradeDialog(runtime, object, 0x3cu, 3u, name,
                            SmithWindowTradeCallback::PageChangeConfirm);
        } else {
            ShowSmithTradeDialog(runtime, object, 0x5eu, 3u, {},
                            SmithWindowTradeCallback::PageChangeConfirm);
        }
        return 1u;
    }
    if (runtime.EquipmentPropertyMatches(equipment_id, prescription_id)) {
        const std::string name(runtime.ItemDataName(
            runtime.EquipmentHoverItemId(object)));
        ShowSmithTradeDialog(runtime, object, 0xccu, 4u, name);
    } else {
        const std::string name(runtime.ItemDataName(
            runtime.EquipmentHoverItemId(object)));
        ShowSmithTradeDialog(runtime, object, 0x3eu, 3u, name,
                        active ? SmithWindowTradeCallback::ExtendedCraftConfirm
                               : SmithWindowTradeCallback::PageChangePrompt);
    }
    return 1u;
}

}  // namespace pal4
