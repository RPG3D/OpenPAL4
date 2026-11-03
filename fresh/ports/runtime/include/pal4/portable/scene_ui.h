#pragma once
#include <cstdint>
#include <iosfwd>
#include <string>

namespace pal4::portable {
// CPU visibility/input state; original HUD drawing belongs to the shared compositor.
struct SceneUi {
    std::uint32_t game_state{1},scene_events{},interaction_resets{};
    std::uint32_t help_topic{}; // UIFrameManager+0x144; original HUD show sets1.
    bool hud_visible{},jump_hint_visible{},progress_visible{};
    bool minimap_expanded{true}; // Original HUD constructor's +0xf5, retained across scene loads.
    std::string cursor{"default.cur"};
    void ShowScene(bool visible);
    void HideInteraction();
};
void WriteSceneUiReport(std::ostream& output,const SceneUi& ui);
} // namespace pal4::portable
