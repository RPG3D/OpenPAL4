#pragma once

#include "pal4/minimap_scene_ui.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

struct MinimapFlashColor {
    float red{};
    float green{};
    float blue{};
    float alpha{};
};

class IMinimapSceneUiStatsDataRuntime {
public:
    virtual ~IMinimapSceneUiStatsDataRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t PlayerStat(std::uint32_t state, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::int32_t PlayerMaxStat(std::uint32_t state, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::int32_t PlayerMode(std::uint32_t state) = 0;
};

class IMinimapSceneUiStatsRuntime : public IMinimapSceneUiStatsDataRuntime {
public:
    virtual ~IMinimapSceneUiStatsRuntime() = default;
    virtual void InitializeNameBuffer() = 0;
    [[nodiscard]] virtual std::int32_t FloatToLong(double value) = 0;
    [[nodiscard]] virtual std::string FormatName(
        std::string_view format, std::int32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t GetImage(
        std::string_view imageset, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t CurrentImage(
        std::uint32_t window) = 0;
    virtual void SetImage(std::uint32_t window,
                          std::uint32_t image) = 0;
    [[nodiscard]] virtual bool FlashFlag() = 0;
    virtual void SetFlashFlag(bool value) = 0;
    virtual void ApplyFlashColor(std::uint32_t window,
                                 MinimapFlashColor color) = 0;
    virtual void RequestRedraw(std::uint32_t window) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupNameBuffer() = 0;
};

class MinimapSceneUiStats final {
public:
    static constexpr std::string_view kImageset = "zhujiemian0";

    [[nodiscard]] static std::uint32_t Update(
        const MinimapSceneUiState& state,
        IMinimapSceneUiStatsRuntime& runtime);
};

}  // namespace pal4
