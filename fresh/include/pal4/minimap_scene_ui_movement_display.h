#pragma once

#include "pal4/minimap_scene_ui.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

enum class MinimapButtonImageState {
    Normal,
    Highlight,
    Disabled,
    Pushed,
};

class IMinimapSceneUiMovementDisplayRuntime {
public:
    virtual ~IMinimapSceneUiMovementDisplayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ActorControl() = 0;
    [[nodiscard]] virtual std::int32_t ActorControlMode(
        std::uint32_t control) = 0;
    [[nodiscard]] virtual std::uint32_t GetImage(
        std::string_view imageset, std::string_view name) = 0;
    virtual void SetButtonImage(std::uint32_t button,
                                std::string_view part,
                                MinimapButtonImageState state,
                                std::uint32_t image) = 0;
    virtual void RequestRedraw(std::uint32_t button) = 0;
};

class MinimapSceneUiMovementDisplay final {
public:
    [[nodiscard]] static void Update(
        const MinimapSceneUiState& state,
        IMinimapSceneUiMovementDisplayRuntime& runtime);
};

}  // namespace pal4
