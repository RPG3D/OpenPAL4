#include "pal4/equipment_item_click_dialog.h"

#include <cstdint>
#include <string>

namespace pal4 {

bool EquipmentItemClickDialog::Open(
    EquipmentWindowState& state, std::uint32_t event_data,
    IEquipmentItemClickRuntime& runtime) {
    const auto character = state.Read(0xf4u);
    if (character == 4u) return true;

    const auto item = runtime.HoveredItem(state);
    if (item == 0u) return true;

    const auto dialog = runtime.Dialog();
    runtime.HideAndCleanup(dialog);
    bool held_by_active_player = false;
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto player = runtime.Player(index);
        if (player != 0u &&
            runtime.ReadWord(player + 0xb08u) != 0u &&
            runtime.ContainsEquipment(
                player, runtime.ReadWord(item + 0x190u))) {
            held_by_active_player = true;
            break;
        }
    }

    const auto selected_player = runtime.Player(character);
    const auto level = runtime.CharacterLevel(selected_player, 4u);
    bool level_allowed = false;
    if (character < 4u) {
        const auto required = static_cast<std::int32_t>(
            runtime.ReadWord(item + 0x168u + 4u * character));
        level_allowed = required > 0 && level >= required;
    }

    const auto item_id = runtime.ReadWord(item + 0x190u);
    const std::string use_text = runtime.ConvertToUtf8(
        runtime.TriggerText(0x18u));
    runtime.CreateButton(dialog, {item_id, "use_equipment", use_text,
                                  level_allowed && !held_by_active_player});

    const std::string discard_text = runtime.ConvertToUtf8(
        runtime.TriggerText(0x17u));
    runtime.CreateButton(dialog, {item_id, "discard_equipment",
                                  discard_text, !held_by_active_player});
    runtime.ShowDialog(dialog, event_data);
    return true;
}

void EquipmentItemClickDialog::CleanupString(
    std::uint32_t object, IEquipmentItemClickRuntime& runtime) {
    runtime.DestroyCeguiString(object + 0xcu);
}

}  // namespace pal4
