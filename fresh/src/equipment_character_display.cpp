#include "pal4/equipment_character_display.h"

#include <array>

namespace pal4 {

void EquipmentCharacterDisplay::SetCharacterImage(
    EquipmentWindowState& state, std::uint32_t character,
    IEquipmentCharacterDisplayRuntime& runtime) {
    static constexpr std::array<std::string_view, 4> kImages = {
        "yuntianhe", "hanlingsha", "liumengli", "murongziying"};
    if (character < kImages.size())
        runtime.SetImage(state.Read(0x44u), "zi0", kImages[character]);
}

void EquipmentCharacterDisplay::UpdateEquipment(
    EquipmentWindowState& state, std::uint32_t character,
    IEquipmentCharacterDisplayRuntime& runtime) {
    if (character == 4u) {
        for (std::uint32_t index = 0u; index < 6u; ++index) {
            const auto window = state.Read(0x98u + index * 4u);
            runtime.SetText(window, "");
            runtime.SetEnabled(window, false);
        }
        return;
    }

    const auto player = runtime.Player(character);
    if (player == 0u) return;
    for (std::uint32_t index = 0u; index < 6u; ++index) {
        const auto window = state.Read(0x98u + index * 4u);
        const auto slot = runtime.ReadWord(window + 0x118u);
        const auto item_id = slot <= 5u ?
            runtime.ReadWord(player + 0x920u + slot * 4u) : slot;
        const auto item = runtime.InventoryItem(item_id, 7u);
        if (item == 0u) {
            runtime.SetText(window, "");
            runtime.SetEnabled(window, false);
            continue;
        }
        const auto name = runtime.ReadWord(item + 0x9cu);
        runtime.SetText(window, name == 0u ? std::string{} :
                        runtime.ItemText(name));
        runtime.SetEnabled(window, index != 0u);
    }
}

}  // namespace pal4
