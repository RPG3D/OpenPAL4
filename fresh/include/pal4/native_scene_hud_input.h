#pragma once

#include "pal4/minimap_scene_ui_input.h"

#include <cstdint>
#include <functional>
#include <string_view>

namespace pal4 {
struct PalActorControlState;
struct PlayerManagerObjectState;
class IMinimapSceneUiInputRuntime;
class IMinimapSceneUiMenuRuntime;
class NativeArenaLoadRuntime;

// State transition and event routing only. Page consumers belong to the host.
class NativeSceneHudMenuRuntime final : public IMinimapSceneUiMenuRuntime {
public:
    using Event = std::function<void(std::uint32_t, std::uint32_t, MinimapSceneUiEventData)>;
    NativeSceneHudMenuRuntime(NativeArenaLoadRuntime& states, std::uint32_t manager,
                              std::uint32_t hud_owner, Event event);
    void SwitchSystemState(std::uint32_t owner) override;
    std::uint32_t FrameManager() override;
    void DispatchEvent(std::uint32_t dispatcher, std::uint32_t key,
                       MinimapSceneUiEventData data) override;
private:
    NativeArenaLoadRuntime& states_;
    std::uint32_t manager_, hud_owner_;
    Event event_;
};

// Borrowed for one original HUD slot0 invocation. The mode callback runs the
// controller's conditional setter; the sound callback uses the audio owner.
// Remaining menu/skill/team services may be supplied separately. Requesting an
// unbound service is an error, never a successfully consumed no-op.
struct NativeSceneHudInputServices {
    PalActorControlState& control;
    PlayerManagerObjectState& players;
    std::function<void(std::int32_t)> set_mode;
    std::function<void(std::string_view, std::uint32_t)> play_sound;
    IMinimapSceneUiInputRuntime* remaining = nullptr;
    // Borrowed; takes precedence over remaining for menu entry only.
    IMinimapSceneUiMenuRuntime* menu = nullptr;
    // HUD V: selected slot's original+A48 character/action id.
    std::function<void(std::uint32_t)> place_signpost;
};
}  // namespace pal4
