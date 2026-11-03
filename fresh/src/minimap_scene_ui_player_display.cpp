#include "pal4/minimap_scene_ui_player_display.h"

#include <array>
#include <bit>

namespace pal4 {
namespace {

constexpr std::array<MinimapButtonImageState, 3> kStates{
    MinimapButtonImageState::Normal,
    MinimapButtonImageState::Highlight,
    MinimapButtonImageState::Pushed,
};

constexpr std::array<std::array<std::string_view, 3>, 4> kMapImages{{
    {{"dituanniuxiangzi1", "dituanniuxiangzi1_2", "dituanniuxiangzi1"}},
    {{"dituanniuxiangzi2", "dituanniuxiangzi2_2", "dituanniuxiangzi2"}},
    {{"dituanniuxiangzi4", "dituanniuxiangzi4_2", "dituanniuxiangzi4"}},
    {{"dituanniuxiangzi3", "dituanniuxiangzi3_2", "dituanniuxiangzi3"}},
}};

constexpr float FloatBits(const std::uint32_t bits) {
    return std::bit_cast<float>(bits);
}

}  // namespace

void MinimapSceneUiPlayerDisplay::Update(
    const MinimapSceneUiState& state,
    IMinimapSceneUiPlayerDisplayRuntime& runtime) {
    const auto manager = runtime.PlayerManager();
    const auto index = runtime.ReadWord(manager + 0x2c50u);
    const auto player = manager + 2836u * index;
    const auto icon_button = state.ReadWord(0xb4u);
    const auto map_button = state.ReadWord(0x68u);

    for (std::uint32_t variant = 3u; variant <= 5u; ++variant) {
        const auto action = runtime.ReadWord(player + 0xa48u);
        const auto image = runtime.GetIconImage(action, variant, 0u);
        runtime.SetButtonImage(icon_button, "Self", kStates[variant - 3u],
                               image);
    }

    const auto action = runtime.ReadWord(player + 0xa48u);
    if (action < kMapImages.size()) {
        const auto& images = kMapImages[action];
        for (std::size_t image_index = 0; image_index < images.size();
             ++image_index) {
            const auto image = runtime.GetImage(kImageset,
                                                images[image_index]);
            runtime.SetButtonImage(map_button, "Self",
                                   kStates[image_index], image);
        }
    }

    const auto mode = runtime.PlayerMode(player);
    MinimapFlashColor color{1.0f, 1.0f, 1.0f, 1.0f};
    if (mode == 1) {
        color = {FloatBits(0x3f139394u), FloatBits(0x3f39b9bau),
                 FloatBits(0x3f5ededfu), 1.0f};
    } else if (mode == 2) {
        color = {FloatBits(0x3f33b3b4u), FloatBits(0x3f4ececfu),
                 FloatBits(0x3f69e9eau), 1.0f};
    }
    runtime.ApplyColor(icon_button, color);
    runtime.RequestRedraw(icon_button);
}

}  // namespace pal4
