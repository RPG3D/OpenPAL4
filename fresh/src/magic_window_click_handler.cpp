#include "pal4/magic_window_click_handler.h"

#include <bit>

namespace pal4 {

bool MagicWindowClickHandler::Handle(
    std::uint32_t object, std::uint32_t clicked_button,
    const MagicWindowState& state, const MagicWindowPanelState& panel,
    IMagicWindowClickRuntime& runtime) {
    const auto key = runtime.ButtonKey(clicked_button);
    const auto category = state.slot_order.at(state.category_position);
    const auto skill = runtime.FindSkill(key);
    const auto player = runtime.GetPlayer(state.selected_character_index);
    if (player == 0 || !skill)
        return true;

    std::int32_t row = 0;
    for (std::uint32_t index = 0; index < 6u; ++index) {
        if (runtime.ButtonKey(panel.At(0xf0u + 4u * index)) == key) {
            row = static_cast<std::int32_t>(index);
            break;
        }
    }

    const auto threshold = std::bit_cast<std::int32_t>(skill->threshold_3c);
    if (runtime.SkillValue(player, category, key) >= threshold) {
        runtime.UpdateDetails(object, row);
        auto callback = runtime.AllocateCallback(0x18u);
        if (callback != 0) {
            runtime.WriteWord(callback + 8u, kCastCallbackHandler);
            runtime.WriteWord(callback + 0xcu, 0u);
            runtime.WriteWord(callback + 0x10u, object);
            runtime.WriteWord(callback, kCastCallbackVtable);
        }
        try {
            runtime.ShowCastDialog(true, callback, key);
        } catch (...) {
            if (callback != 0) runtime.DeleteCallback(callback, 1u);
            throw;
        }
        if (callback != 0) runtime.DeleteCallback(callback, 1u);
        return true;
    }

    if (runtime.RemainingValue(player) < threshold)
        return true;

    runtime.AddSkillDelta(player, category, key, skill->threshold_3c);
    runtime.UpdatePlayerCollection(player, category, skill->threshold_3c);
    const auto stat_base = category < 5u
        ? runtime.PlayerStatBase(player, category) : category;
    runtime.SetPlayerStat(player, category, stat_base + skill->threshold_3c);

    if (runtime.SkillValue(player, category, key) >= threshold) {
        runtime.InsertLearnedSkill(player, category, key);
        runtime.ShowLearningHint(skill->name_address, 9u, 0.5f, 0.4f);
        runtime.PlaySound("gamedata\\palSound\\wa08.wav");
    }
    runtime.InitializeImages(object);
    runtime.UpdateDetails(object, row);
    return true;
}

}  // namespace pal4
