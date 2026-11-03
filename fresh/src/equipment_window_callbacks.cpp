#include "pal4/equipment_window_callbacks.h"

#include <bit>
#include <cstdint>

namespace pal4 {
namespace {

void RefreshSelection(EquipmentWindowState& state,
                      IEquipmentWindowCallbacksRuntime& runtime) {
    runtime.UpdateList(state, state.Read(0xf0u), state.Read(0xf4u));
    const auto item = EquipmentWindowCallbacks::HoveredItem(state, runtime);
    runtime.UpdateDisplay(state, item, state.Read(0xf4u));
}

bool ChangeCharacter(EquipmentWindowState& state, bool previous,
                     IEquipmentWindowCallbacksRuntime& runtime) {
    const auto start = state.Read(0xf4u);
    for (;;) {
        const auto next = previous ?
            runtime.PreviousCharacterIndex(state.Read(0xf4u)) :
            runtime.NextCharacterIndex(state.Read(0xf4u));
        state.Write(0xf4u, next);
        if (next == start) return true;
        if (next != 4u) {
            const auto player = runtime.Player(next);
            if (player == 0u || runtime.ReadWord(player + 0xb08u) == 0u)
                continue;
        }
        runtime.RemoveMember(0x10cu);
        runtime.RemoveMember(0x118u);
        runtime.RefreshSecondaryMember(0x118u);
        runtime.InsertMember(0x10cu);
        RefreshSelection(state, runtime);
        return true;
    }
}

}  // namespace

std::int32_t EquipmentCharacterIndex::Next(std::int32_t current) noexcept {
    const auto next = std::bit_cast<std::int32_t>(
        static_cast<std::uint32_t>(current) + 1u);
    return next >= 4 ? 0 : next;
}

std::int32_t EquipmentCharacterIndex::Previous(
    std::int32_t current) noexcept {
    const auto previous = std::bit_cast<std::int32_t>(
        static_cast<std::uint32_t>(current) - 1u);
    return previous < 0 ? 3 : previous;
}

std::uint32_t EquipmentWindowCallbacks::HoveredItem(
    EquipmentWindowState& state,
    IEquipmentWindowHoverRuntime& runtime) {
    const auto row = runtime.HoveredRow(state.Read(0x40u));
    if (row == 0u) return 0u;
    return runtime.InventoryItem(runtime.ReadWord(row + 0x138u), 7u);
}

bool EquipmentWindowCallbacks::HandleMouseClick(
    EquipmentWindowState& state, std::uint32_t event,
    IEquipmentWindowCallbacksRuntime& runtime) {
    runtime.HandleItemClick(state, event + 0xcu);
    return true;
}

bool EquipmentWindowCallbacks::HandleHoverChange(
    EquipmentWindowState& state,
    IEquipmentWindowCallbacksRuntime& runtime) {
    const auto item = HoveredItem(state, runtime);
    runtime.UpdateItemDetails(state, item);
    if (item != 0u) runtime.UpdateItemModel(state, item);
    return true;
}

bool EquipmentWindowCallbacks::HandleCharacterClick(
    EquipmentWindowState& state, std::uint32_t event,
    IEquipmentWindowCallbacksRuntime& runtime) {
    const auto window = runtime.ReadWord(event + 8u);
    return SelectCategory(state, runtime.ReadWord(window + 0x118u), runtime);
}

bool EquipmentWindowCallbacks::SelectCategory(
    EquipmentWindowState& state, std::uint32_t category,
    IEquipmentWindowCallbacksRuntime& runtime) {
    runtime.UpdateList(state, category, state.Read(0xf4u));
    const auto item = HoveredItem(state, runtime);
    runtime.UpdateDisplay(state, item, state.Read(0xf4u));
    runtime.SetImage(state.Read(0xb0u), state.Read(0xc8u + 4u * category));
    runtime.InsertMember(0x10cu);
    return true;
}

bool EquipmentWindowCallbacks::SelectPreviousCharacter(
    EquipmentWindowState& state,
    IEquipmentWindowCallbacksRuntime& runtime) {
    return ChangeCharacter(state, true, runtime);
}

bool EquipmentWindowCallbacks::SelectNextCharacter(
    EquipmentWindowState& state,
    IEquipmentWindowCallbacksRuntime& runtime) {
    return ChangeCharacter(state, false, runtime);
}

bool EquipmentWindowCallbacks::Unequip(
    EquipmentWindowState& state, std::uint32_t event,
    IEquipmentWindowUnequipRuntime& runtime) {
    const auto window = runtime.ReadWord(event + 8u);
    const auto slot = runtime.ReadWord(window + 0x118u);
    const auto player = runtime.Player(state.Read(0xf4u));
    if (player != 0u) {
        runtime.PlaySound("wa13.wav", 1u);
        if (slot <= 5u) runtime.WriteWord(player + 0x920u + 4u * slot, 0u);
        runtime.RefreshPlayer(player);
        const auto item = HoveredItem(state, runtime);
        runtime.UpdateDisplay(state, item, state.Read(0xf4u));
        const auto list = state.Read(0x40u);
        const auto row = runtime.HoverRowIndex(list);
        const auto position = runtime.ScrollPosition(list);
        runtime.UpdateList(state, state.Read(0xf0u), state.Read(0xf4u));
        runtime.SetScrollPosition(list, position);
        runtime.SetHoverRowIndex(list, row);
    }
    return true;
}

bool EquipmentWindowCallbacks::HandleCharacterSelection(
    EquipmentWindowState& state, std::uint32_t event,
    IEquipmentWindowCallbacksRuntime& runtime) {
    const auto value = runtime.ReadWord(event + 0x10u);
    if (value == 2u) return SelectCategory(state, 5u, runtime);
    if (value < 3u || value >= 8u) return false;
    return SelectCategory(state, value - 3u, runtime);
}

}  // namespace pal4
