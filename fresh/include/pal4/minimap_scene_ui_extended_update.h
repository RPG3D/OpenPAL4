#pragma once

#include "pal4/minimap_scene_ui.h"

#include <cstdint>
#include <optional>
#include <utility>

namespace pal4 {
// IDA 47AC5D..47AC90: unhandled scene modes retain the current map mode.
constexpr std::optional<std::uint32_t> MinimapModeForSceneType(std::uint32_t type){
    if(type>2)return {};
    return type==0?1u:type==1?0u:2u;
}

struct MinimapSceneTriggerVariable {
    std::uint32_t primary{};
    std::uint32_t secondary{};
};

class IMinimapSceneUiExtendedRuntime {
public:
    virtual ~IMinimapSceneUiExtendedRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    [[nodiscard]] virtual std::uint32_t MinimapManager() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void LoadScriptData(std::uint32_t minimap,
                                std::uint32_t arena_path,
                                std::uint32_t arena_script) = 0;
    virtual void InitializeDialogSystem() = 0;
    [[nodiscard]] virtual MinimapSceneTriggerVariable QueryTriggerVariable(
        std::uint32_t arena_path, std::uint32_t variable_name) = 0;
    [[nodiscard]] virtual std::uint32_t CopyFloorAttribute(
        std::uint32_t source) = 0;
    virtual void ReleaseFloorAttribute(std::uint32_t copy) = 0;
    virtual void ReleaseTriggerVariable(
        MinimapSceneTriggerVariable variable) = 0;
    [[nodiscard]] virtual std::uint32_t ItemPropertyTable() = 0;
    [[nodiscard]] virtual std::uint32_t ItemPropertyCount(
        std::uint32_t collection) = 0;
    [[nodiscard]] virtual bool MatchesFloorAttribute(
        std::uint32_t key, std::uint32_t row) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyString() = 0;
    [[nodiscard]] virtual std::uint32_t ConvertToUtf8(
        std::uint32_t text) = 0;
    virtual void SetWindowText(std::uint32_t window,
                               std::uint32_t utf8_text) = 0;
    virtual void SetFlagAndUpdate(std::uint32_t owner,
                                  std::uint8_t value) = 0;
    virtual void UpdateVisibilityAndMinimap(std::uint32_t owner) = 0;
    virtual void UpdateVisibilityBasedOnState(std::uint32_t owner) = 0;
    [[nodiscard]] virtual std::pair<std::int32_t, std::int32_t>
        ConfigSize() = 0;
    [[nodiscard]] virtual std::int32_t FloatToLong(double value) = 0;
    virtual void SetupMinimapTexture(
        std::uint32_t minimap, std::int32_t x,
        std::int32_t y, std::int32_t width,
        std::int32_t height) = 0;
};

class MinimapSceneUiExtendedUpdate final {
public:
    [[nodiscard]] static std::uint8_t Update(
        MinimapSceneUiState& state, std::uint32_t owner,
        IMinimapSceneUiExtendedRuntime& runtime);
};

}  // namespace pal4
