#include "pal4/equipment_window_actions.h"

#include <bit>

namespace pal4 {

bool EquipmentWindowActions::Equip(
    EquipmentWindowState& state, std::uint32_t item_id,
    IEquipmentWindowMutationRuntime& runtime) {
    const auto character = state.Read(0xf4u);
    const auto player = runtime.Player(character);
    if (player == 0u) return false;
    const auto item = runtime.InventoryItem(item_id, 7u);
    if (item == 0u) return false;

    runtime.PlaySound("wa13.wav", 1u);
    const auto slot = runtime.EquipmentSlot(player, item);
    if (slot <= 5u)
        runtime.WriteWord(player + 0x920u + slot * 4u,
                          runtime.ReadWord(item + 0x190u));
    runtime.RefreshPlayer(player);
    runtime.UpdateList(state, state.Read(0xf0u), character);
    runtime.UpdateDisplay(state, item, character);
    return true;
}

bool EquipmentWindowActions::ShowDiscardConfirmation(
    EquipmentWindowState& state, std::uint32_t object_address,
    std::uint32_t item_id, IEquipmentWindowMutationRuntime& runtime) {
    if (runtime.Player(state.Read(0xf4u)) == 0u) return false;
    const auto item = runtime.InventoryItem(item_id, 7u);
    if (item == 0u) return false;

    const auto text_address = runtime.ReadWord(item + 0x9cu);
    const auto item_text = text_address == 0u ? std::string{} :
        runtime.ItemText(text_address);
    const auto format = runtime.ItemInfoFormat(0x6au);
    const auto text = runtime.FormatItemInfo(format, item_text);
    const auto callback = runtime.AllocateCallback(0x18u);
    if (callback != 0u)
        runtime.InitializeDiscardCallback(callback, object_address);
    try {
        constexpr float kPosition = std::bit_cast<float>(0x3e99999au);
        runtime.ShowCommonDialog(text, kPosition, kPosition, 3u, callback);
    } catch (...) {
        if (callback != 0u) runtime.ReleaseCallback(callback, 1u);
        throw;
    }
    if (callback != 0u) runtime.ReleaseCallback(callback, 1u);
    return true;
}

bool EquipmentWindowActions::ConfirmDiscard(
    EquipmentWindowState& state, std::uint32_t answer,
    IEquipmentWindowMutationRuntime& runtime) {
    if (answer == 2u) return true;
    if (runtime.Player(state.Read(0xf4u)) == 0u) return false;
    const auto hovered = runtime.HoveredItem(state);
    if (hovered == 0u) return false;

    runtime.PlaySound("wa07.wav", 1u);
    runtime.RemoveInventoryItem(hovered);
    runtime.UpdateList(state, state.Read(0xf0u), state.Read(0xf4u));
    const auto next = runtime.HoveredItem(state);
    if (next == 0u) return false;
    runtime.UpdateDisplay(state, next, state.Read(0xf4u));
    return true;
}

void EquipmentWindowActions::UpdateList(
    EquipmentWindowState& state, std::uint32_t category,
    std::uint32_t character, IEquipmentWindowPresentationRuntime& runtime) {
    runtime.ClearItemDetails(state);
    runtime.SetEquipmentVisibility(state, true);
    state.Write(0xf0u, category);
    state.Write(0xf4u, character);
    for (std::uint32_t i = 0u; i < 4u; ++i)
        runtime.SetVisible(state.Read(0xe0u + i * 4u), false);
    if (character < 4u) {
        runtime.SetVisible(state.Read(0xe0u + character * 4u), true);
        const auto player = runtime.Player(character);
        if (player != 0u)
            runtime.SetText(state.Read(0xb4u),
                            runtime.FormatLevel(runtime.CharacterLevel(player, 4u)));
    }

    const auto list = state.Read(0x40u);
    runtime.SetSortDirection(list, 0u);
    runtime.ResetList(list);
    runtime.PopulateList(state, category);
    if (runtime.RowCount(list) != 0u) runtime.SetHoverRow(list, 0u);
    runtime.SetCharacterImage(state, character);
    for (std::uint32_t i = 0u; i < 6u; ++i)
        runtime.SetSelected(state.Read(0x50u + i * 4u), false);
    runtime.SetSelected(state.Read(0x50u + category * 4u), true);
    runtime.UpdateCharacterEquipment(state, character);
}

void EquipmentWindowActions::UpdateDisplay(
    EquipmentWindowState& state, std::uint32_t item,
    std::uint32_t character, IEquipmentWindowPresentationRuntime& runtime) {
    if (item != 0u) runtime.UpdateCharacterEquipment(state, character);
    runtime.UpdateItemDetails(state, item);
}

}  // namespace pal4
