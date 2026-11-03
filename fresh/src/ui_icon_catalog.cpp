#include "pal4/ui_icon_catalog.h"

#include <array>

namespace pal4 {
namespace {

using Names = std::array<std::string_view, 4>;
struct Category {
    std::string_view imageset;
    Names active;
    Names inactive;
};

constexpr std::array<Category, 7> kCategories{{
    {"gy0", {"tou1_2", "tou2_2", "tou4_2", "tou3"},
             {"tou1", "tou2", "tou4", "tou3"}},
    {"gy0", {"zhandoutianhe2", "zhandoulinsha2",
              "zhandoumengli2", "zhandouziying"},
             {"zhandoutianhe", "zhandoulinsha",
              "zhandoumengli", "zhandouziying"}},
    {"gy0", {"tou1bb_2", "tou2bb_2", "tou4bb_2", "tou3bb"},
             {"tou1bb", "tou2bb", "tou4bb", "tou3bb"}},
    {"zhujiemian0",
        {"touxianganniu1_2", "touxianganniua1_2",
         "touxianganniuc1_2", "touxianganniud1"},
        {"touxianganniu1", "touxianganniua1",
         "touxianganniuc1", "touxianganniud1"}},
    {"zhujiemian0",
        {"touxianganniu2_2", "touxianganniub2_2",
         "touxianganniuc2_2", "touxianganniud2"},
        {"touxianganniu2", "touxianganniub2",
         "touxianganniuc2", "touxianganniud2"}},
    {"zhujiemian0",
        {"touxianganniu3_2", "touxianganniub3_2",
         "touxianganniuc3_2", "touxianganniud3"},
        {"touxianganniu3", "touxianganniub3",
         "touxianganniuc3", "touxianganniud3"}},
    {"gy0", {"tou1b_2", "tou2b_2", "tou4b_2", "tou3b"},
             {"tou1b", "tou2b", "tou4b", "tou3b"}},
}};

}  // namespace

std::uint32_t UiIconCatalog::GetIconImage(
    const std::int32_t character, const std::int32_t category,
    const std::int32_t player_override, IUiIconCatalogRuntime& runtime) {
    bool player102_active;
    if (player_override != 0) {
        player102_active = player_override == 2;
    } else {
        runtime.InitializePlayerManager();
        player102_active = runtime.IsPlayer102Active();
    }
    return GetIconImageForPlayerState(character, category, player102_active, runtime);
}

std::uint32_t UiIconCatalog::GetIconImageForPlayerState(
    const std::int32_t character, const std::int32_t category,
    const bool player102_active, IUiIconImagesRuntime& runtime) {
    if (category < 0 || category >= static_cast<std::int32_t>(kCategories.size()) ||
        character < 0 || character >= 4)
        return 0u;

    const auto& selected = kCategories[static_cast<std::size_t>(category)];
    const auto& names = player102_active ? selected.active : selected.inactive;
    const auto imageset = runtime.GetImageset(selected.imageset);
    return runtime.GetImage(imageset,
                            names[static_cast<std::size_t>(character)]);
}

}  // namespace pal4
