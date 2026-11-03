#include "pal4/minimap_scene_ui_movement_display.h"

#include <array>

namespace pal4 {

void MinimapSceneUiMovementDisplay::Update(
    const MinimapSceneUiState& state,
    IMinimapSceneUiMovementDisplayRuntime& runtime) {
    constexpr std::array<std::array<std::string_view, 3>, 3> names{{
        {{"zou", "zou2", "zou3"}},
        {{"pao", "pao2", "pao3"}},
        {{"kuaipao", "kuaipao2", "kuaipao3"}},
    }};
    const auto mode = runtime.ActorControlMode(runtime.ActorControl());
    const auto button = state.ReadWord(0xb8u);
    if (mode >= 0 && mode < static_cast<std::int32_t>(names.size())) {
        const auto& mode_names = names[static_cast<std::size_t>(mode)];
        const auto normal = runtime.GetImage("zhujiemian0", mode_names[0]);
        runtime.SetButtonImage(button, "Self",
                               MinimapButtonImageState::Normal, normal);
        const auto highlight = runtime.GetImage("zhujiemian0", mode_names[1]);
        runtime.SetButtonImage(button, "Self",
                               MinimapButtonImageState::Highlight, highlight);
        const auto inactive = runtime.GetImage("zhujiemian0", mode_names[2]);
        runtime.SetButtonImage(button, "Self",
                               MinimapButtonImageState::Disabled, inactive);
        runtime.SetButtonImage(button, "Self",
                               MinimapButtonImageState::Pushed, inactive);
    }
    runtime.RequestRedraw(button);
}

}  // namespace pal4
