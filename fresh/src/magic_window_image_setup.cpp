#include "pal4/magic_window_image_setup.h"

#include <array>
#include <string_view>

namespace pal4 {

std::uint32_t MagicWindowImageSetup::Prepare(
    std::uint32_t object, const MagicWindowState& state,
    const MagicWindowPanelState& panel,
    IMagicWindowImageSetupRuntime& runtime) {
    runtime.SetText(panel.At(0x88u), "");
    runtime.SetText(panel.At(0x8cu), "");

    constexpr std::array<std::string_view, 5> small_images{
        "5water_s", "5fire_s", "5thurder_s", "5wind_s", "5earth_s"};
    constexpr std::array<std::string_view, 5> large_images{
        "5water_l", "5fire_l", "5thurder_l", "5wind_l", "5earth_l"};
    for (std::uint32_t i = 0; i < small_images.size(); ++i)
        runtime.SetImage(panel.At(0x140u + 4u * i), "maimai0", small_images[i]);

    const auto element = state.slot_order.at(state.category_position);
    if (element < large_images.size())
        runtime.SetImage(panel.At(0x140u + 4u * element),
                         "maimai0", large_images[element]);

    constexpr std::array<std::uint32_t, 6> visible_labels{
        0x94u, 0x90u, 0x98u, 0xa0u, 0x9cu, 0xa4u};
    for (const auto offset : visible_labels)
        runtime.SetVisible(panel.At(offset), true);
    runtime.SetStuntPanelVisible(object, true);
    runtime.SetImage(panel.At(0x154u), "zi0", "xianshu");

    const auto player = runtime.GetPlayer(state.selected_character_index);
    if (player != 0) {
        runtime.SetMagicPanelVisible(object, true);
        runtime.ResetStuntPanel(object);
    }
    return player;
}

}  // namespace pal4
