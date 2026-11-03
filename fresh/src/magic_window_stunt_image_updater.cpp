#include "pal4/magic_window_stunt_image_updater.h"

#include "pal4/magic_window_skill_catalog.h"

#include <array>

namespace pal4 {

std::size_t MagicWindowStuntImageUpdater::Update(
    std::uint32_t object, const MagicWindowState& state,
    const MagicWindowPanelState& panel,
    IMagicWindowStuntImageRuntime& runtime) {
    runtime.SetText(panel.At(0x88u), "");
    runtime.SetText(panel.At(0x8cu), "");

    constexpr std::array<std::string_view, 5> small_images{
        "5water_s", "5fire_s", "5thurder_s", "5wind_s", "5earth_s"};
    for (std::uint32_t index = 0; index < small_images.size(); ++index)
        runtime.SetImage(panel.At(0x140u + 4u * index), "maimai0", small_images[index]);

    runtime.SetStuntPanelVisible(object, true);
    runtime.SetImage(panel.At(0x154u), "zi0", "teji");
    constexpr std::array<std::uint32_t, 6> label_offsets{
        0x94u, 0x90u, 0x98u, 0xa0u, 0x9cu, 0xa4u};
    for (const auto offset : label_offsets)
        runtime.SetVisible(panel.At(offset), false);

    const auto player = runtime.GetPlayer(state.selected_character_index);
    if (player == 0)
        return 0;

    runtime.SetMagicPanelVisible(object, false);
    for (std::uint32_t row = 0; row < kVisibleLimit; ++row)
        runtime.SetVisible(panel.At(0x108u + 4u * row), false);

    std::size_t visible = 0;
    for (const auto key : runtime.StuntKeys(player)) {
        if (visible == kVisibleLimit)
            break;
        const auto record = runtime.FindStunt(key);
        if (!record)
            continue;

        const auto row = static_cast<std::uint32_t>(visible);
        const auto button = panel.At(0x108u + 4u * row);
        const auto icon = panel.At(0x124u + 4u * row);
        runtime.SetText(button, runtime.ReadNameUtf8(record->name_address));
        runtime.SetButtonKey(button, key);
        runtime.SetVisible(button, true);
        runtime.SetImage(icon, "gy0", MagicWindowSkillCatalog::IconForKind(record->kind));
        ++visible;
    }
    runtime.UpdateStuntDetails(object);
    return visible;
}

}  // namespace pal4
