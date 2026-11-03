#include "pal4/minimap_scene_ui_stats.h"

namespace pal4 {
namespace {

void SetStatImage(const MinimapSceneUiState& state,
                  const std::uint32_t window_offset,
                  const std::string_view format, const std::int32_t value,
                  IMinimapSceneUiStatsRuntime& runtime) {
    const auto name = runtime.FormatName(format, value);
    const auto image = runtime.GetImage(MinimapSceneUiStats::kImageset, name);
    const auto window = state.ReadWord(window_offset);
    if (runtime.CurrentImage(window) != image) {
        runtime.SetImage(window, image);
    }
}

}  // namespace

std::uint32_t MinimapSceneUiStats::Update(
    const MinimapSceneUiState& state,
    IMinimapSceneUiStatsRuntime& runtime) {
    const auto manager = runtime.PlayerManager();
    const auto index = runtime.ReadWord(manager + 0x2c50u);
    const auto player_state = manager + 2836u * index + 0x654u;
    runtime.InitializeNameBuffer();
    try {
        const auto hp = runtime.PlayerStat(player_state, 7u);
        const auto max_hp = runtime.PlayerMaxStat(player_state, 0u);
        auto hp_digit = runtime.FloatToLong(
            static_cast<double>(static_cast<float>(hp)) /
            static_cast<double>(max_hp) * 10.0);
        if (hp_digit > 9) hp_digit = 9;
        SetStatImage(state, 0xc4u, "hp%d", hp_digit, runtime);

        const auto mp = runtime.PlayerStat(player_state, 8u);
        auto mp_digit = runtime.FloatToLong(
            static_cast<double>(mp) * static_cast<double>(0.1f));
        if (mp_digit > 9) mp_digit = 9;
        SetStatImage(state, 0xc8u, "mp%d", mp_digit, runtime);

        if (runtime.PlayerMode(player_state) == 1 ||
            runtime.PlayerMode(player_state) == 2) {
            const auto window = state.ReadWord(0xb4u);
            const auto light = runtime.FlashFlag();
            const MinimapFlashColor color = light
                ? MinimapFlashColor{0.81960785f, 0.80784315f,
                                    0.9137255f, 1.0f}
                : MinimapFlashColor{0.57647061f, 0.56862748f,
                                    0.79215688f, 1.0f};
            runtime.ApplyFlashColor(window, color);
            runtime.SetFlashFlag(!light);
            runtime.RequestRedraw(window);
        }
    } catch (...) {
        (void)runtime.CleanupNameBuffer();
        throw;
    }
    return runtime.CleanupNameBuffer();
}

}  // namespace pal4
