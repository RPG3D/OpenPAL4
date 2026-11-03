#include "pal4/magic_window_character_display.h"

#include <array>
#include <string_view>

namespace pal4 {

void MagicWindowCharacterDisplay::Update(
    MagicWindowState& state, const MagicWindowPanelState& panel,
    std::int32_t character_index,
    IMagicWindowCharacterDisplayRuntime& runtime) {
    state.selected_character_index = character_index;
    const auto player = runtime.GetPlayer(character_index);
    if (!player) return;

    constexpr std::array<std::string_view, 4> portraits{
        "yuntianhe", "hanlingsha", "liumengli", "murongziying"};
    if (player->portrait_id < portraits.size()) {
        runtime.SetPortrait(panel.At(0x7cu), "zi0", portraits[player->portrait_id]);
    }
    if (state.category_position != 0)
        runtime.InitializeMagicImages();
    else
        runtime.UpdateMagicImages();

    std::uint32_t active_count = 0;
    for (std::int32_t index = 0; index < 4; ++index) {
        const auto party_member = runtime.GetPlayer(index);
        if (party_member && party_member->active) ++active_count;
    }
    const auto navigation_visible = active_count > 1u;
    runtime.SetVisible(panel.At(0x80u), navigation_visible);
    runtime.SetVisible(panel.At(0x84u), navigation_visible);
}

}  // namespace pal4
