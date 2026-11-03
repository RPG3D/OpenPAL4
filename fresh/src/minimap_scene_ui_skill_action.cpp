#include "pal4/minimap_scene_ui_skill_action.h"

namespace pal4 {

std::uint8_t MinimapSceneUiSkillAction::Activate(
    const std::uint32_t ignored_event,
    IMinimapSceneUiSkillRuntime& runtime) {
    (void)ignored_event;
    const auto manager = runtime.PlayerManager();
    const auto index = runtime.ReadWord(manager + 0x2c50u);
    const auto player = manager + 2836u * index;
    const auto action = runtime.ReadWord(player + 0xa48u);
    const std::int32_t cost = action == 0u ? 15 : action == 2u ? 20 : 5;
    const auto state = player + 0x654u;
    if (runtime.PlayerStat(state, 8u) >= cost) {
        runtime.AddPlayerStat(state, 8u, -cost);
        runtime.PlayerAction(runtime.Arena(), action);
    } else {
        auto text = runtime.TriggerItemText(0x6cu);
        if (text == 0u) text = runtime.EmptyString();
        const auto utf8 = runtime.ConvertToUtf8(text);
        runtime.ShowHint(utf8, 0x3f000000u, 0x3e19999au,
                         0x40000000u, 0u, 1u);
    }
    return 1u;
}

std::uint8_t MinimapSceneUiSkillAction::UpdateState(
    const std::uint32_t ignored_event,
    IMinimapSceneUiSkillRuntime& runtime) {
    (void)ignored_event;
    const auto manager = runtime.PlayerManager();
    const auto index = runtime.ReadWord(manager + 0x2c50u);
    const auto player = manager + 2836u * index;
    if (player != 0u) {
        const auto action = runtime.ReadWord(player + 0xa48u);
        runtime.PlayerActionSecondary(runtime.Arena(), action);
    }
    return 1u;
}

}  // namespace pal4
