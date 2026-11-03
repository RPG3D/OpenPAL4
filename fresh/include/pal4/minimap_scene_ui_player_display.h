#pragma once

#include "pal4/minimap_scene_ui.h"
#include "pal4/minimap_scene_ui_movement_display.h"
#include "pal4/minimap_scene_ui_stats.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IMinimapSceneUiPlayerDisplayRuntime {
public:
    virtual ~IMinimapSceneUiPlayerDisplayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetIconImage(
        std::uint32_t action, std::uint32_t variant,
        std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t GetImage(
        std::string_view imageset, std::string_view name) = 0;
    virtual void SetButtonImage(std::uint32_t button,
                                std::string_view part,
                                MinimapButtonImageState state,
                                std::uint32_t image) = 0;
    [[nodiscard]] virtual std::int32_t PlayerMode(
        std::uint32_t player) = 0;
    virtual void ApplyColor(std::uint32_t button,
                            MinimapFlashColor color) = 0;
    virtual void RequestRedraw(std::uint32_t button) = 0;
};

class MinimapSceneUiPlayerDisplay final {
public:
    static constexpr std::string_view kImageset = "zhujiemian0";

    static void Update(const MinimapSceneUiState& state,
                       IMinimapSceneUiPlayerDisplayRuntime& runtime);
};

}  // namespace pal4
