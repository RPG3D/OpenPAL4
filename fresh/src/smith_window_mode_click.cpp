#include "pal4/smith_window.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

std::uint8_t SmithWindow::OnCraftModeClick(
    const std::uint32_t object, const std::uint32_t requested_mode,
    ISmithWindowModeClickRuntime& runtime) {
    runtime.ClearEquipmentSelection(object);
    runtime.LoadData(object, requested_mode);

    const auto list = runtime.ReadWord(object + 0x44u);
    if (runtime.RowCount(list) != 0u) runtime.SetHoverRow(list, 0u);

    const auto mode = runtime.ReadWord(object + 0xb4u);
    constexpr std::array<std::string_view, 3> images{
        "rongzhutupu", "duanyetupu", "zhulingtupu"};
    constexpr std::array<std::string_view, 3> textures{
        "ZZA1", "ZZA2", "ZZA3"};
    if (mode > 2u) return 1u;

    runtime.SetImage(runtime.ReadWord(object + 0x70u), "zi0", images[mode]);
    for (std::uint32_t index = 0; index < 3u; ++index) {
        runtime.SetSelected(runtime.ReadWord(object + 0x60u + index * 4u),
                            index == mode);
    }
    runtime.UpdateModelPreview("zza", textures[mode], "PALObject\\");
    return 1u;
}

}  // namespace pal4
