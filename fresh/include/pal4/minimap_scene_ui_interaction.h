#pragma once

#include "pal4/minimap_scene_ui_visibility.h"

#include <cstdint>

namespace pal4 {

struct MinimapPointerEvent {
    std::uint32_t window{};
    float x{};
    float y{};
};

class IMinimapSceneUiInteractionRuntime {
public:
    virtual ~IMinimapSceneUiInteractionRuntime() = default;
    [[nodiscard]] virtual MinimapUiPosition AbsolutePosition(
        std::uint32_t window) = 0;
    [[nodiscard]] virtual MinimapUiPosition TransformCoordinates(
        MinimapUiPosition position) = 0;
    [[nodiscard]] virtual MinimapUiPosition GameStatePosition() = 0;
    [[nodiscard]] virtual std::int32_t FloatToLong(float value) = 0;
    [[nodiscard]] virtual std::uint32_t FindMinimapObject(
        std::int32_t x, std::int32_t y, std::uint32_t mode) = 0;
    virtual void CloseUtf8Window(std::uint32_t address) = 0;
    [[nodiscard]] virtual MinimapUiPosition AbsoluteToRelative(
        MinimapUiPosition position) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FindGameObjectByName(
        std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t TriggerObject(
        std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t TriggerSwitch(
        std::uint32_t trigger_object) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyString() = 0;
    virtual void ConvertUtf8ToUnicode(
        std::uint32_t text, MinimapUiPosition position,
        std::uint32_t scale_bits, std::uint32_t flags,
        std::uint32_t enabled) = 0;
};

class MinimapSceneUiInteraction final {
public:
    [[nodiscard]] static std::uint8_t UpdateDisplay(
        std::uint32_t owner, const MinimapPointerEvent& event,
        IMinimapSceneUiInteractionRuntime& runtime);
};

}  // namespace pal4
