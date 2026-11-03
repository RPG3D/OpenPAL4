#include "pal4/minimap_scene_ui_interaction.h"

namespace pal4 {

std::uint8_t MinimapSceneUiInteraction::UpdateDisplay(
    const std::uint32_t owner, const MinimapPointerEvent& event,
    IMinimapSceneUiInteractionRuntime& runtime) {
    auto absolute = runtime.AbsolutePosition(event.window);
    absolute.x += 2.0f;
    const auto transformed = runtime.TransformCoordinates(absolute);
    const auto game_position = runtime.GameStatePosition();
    const auto delta_x = game_position.x - transformed.x;
    const auto delta_y = game_position.y - transformed.y;
    const auto y = runtime.FloatToLong(delta_y);
    const auto x = runtime.FloatToLong(delta_x);
    const auto minimap_object = runtime.FindMinimapObject(x, y, 1u);
    if (minimap_object == 0u) {
        runtime.CloseUtf8Window(owner + 0x6cu);
        return 1u;
    }

    const auto relative = runtime.AbsoluteToRelative(
        {event.x, event.y - 20.0f});
    const auto name = runtime.ReadWord(minimap_object + 8u);
    const auto game_object = runtime.FindGameObjectByName(name);
    if (game_object == 0u) return 1u;
    const auto trigger_index =
        static_cast<std::int32_t>(runtime.ReadWord(game_object + 0x1bcu));
    if (trigger_index < 0) return 1u;
    const auto trigger_object = runtime.TriggerObject(trigger_index);
    if (trigger_object == 0u) return 1u;
    const auto trigger_switch = runtime.TriggerSwitch(trigger_object);
    auto text = runtime.ReadWord(trigger_switch + 4u);
    if (text == 0u) text = runtime.EmptyString();
    runtime.ConvertUtf8ToUnicode(text, relative, 0x40800000u, 0u, 1u);
    return 1u;
}

}  // namespace pal4
