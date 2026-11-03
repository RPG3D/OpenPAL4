#include "pal4/combat_ending_window_elements.h"

#include <array>
#include <bit>
#include <string>
#include <utility>

namespace pal4 {

void CombatEndingWindowElements::Setup(
    const std::uint32_t dialog,
    ICombatEndingWindowElementsRuntime& runtime) {
    const auto prefix = runtime.LayoutPrefix(dialog + 20u);
    const auto find = [&](const std::string_view suffix) {
        return runtime.FindWindow(dialog, prefix + std::string(suffix));
    };
    const auto store = [&](const std::uint32_t offset,
                           const std::string_view suffix) {
        const auto found = find(suffix);
        runtime.WriteWord(dialog + offset, found);
        return found;
    };

    (void)store(80u, "PanelScoreLv");
    runtime.SetAlwaysOnTop(store(84u, "PanelScoreMark"), true);
    (void)store(64u, "ImgScoreLv");
    (void)store(88u, "PanelPrize");
    (void)store(104u, "EditPrizeLeft");
    (void)store(108u, "EditPrizeRight");
    (void)store(112u, "StaticPrizeTitle");
    runtime.SetFormatting(store(68u, "StaticScoreMark"), 2u, 2u);
    (void)store(72u, "PanelMoney");
    (void)store(76u, "PanelProp");
    const auto money = store(92u, "StaticMoney");
    runtime.SetAlwaysOnTop(money, true);
    runtime.SetHorizontalFormatting(money, 2u);
    for (const auto binding : std::array{
             std::pair{96u, "EditPropLeft"},
             std::pair{100u, "EditPropRight"}}) {
        const auto edit = store(binding.first, binding.second);
        runtime.SetAlwaysOnTop(edit, true);
        runtime.SetClippedByParent(edit, false);
    }

    constexpr std::array<std::pair<std::uint32_t, std::string_view>, 11>
        row_elements{{
            {120u, "StaticName"}, {124u, "StaticHp"},
            {128u, "StaticMp"}, {132u, "StaticRage"},
            {136u, "StaticExp"}, {140u, "StaticNeedExp"},
            {144u, "ImgHp"}, {148u, "ImgMp"},
            {152u, "ImgRage"}, {156u, "ImgRole"},
            {160u, "ImgName"}}};
    for (std::uint32_t i = 0u; i < 4u; ++i) {
        const auto suffix = std::to_string(i);
        const auto row = dialog + 76u * i;
        const auto panel = store(116u + 76u * i,
                                 "PanelRole" + suffix);
        runtime.WriteWord(row + 176u,
                          std::bit_cast<std::uint32_t>(
                              runtime.GetYPosition(panel)));
        for (const auto& [offset, name] : row_elements) {
            const auto found = store(offset + 76u * i,
                                     std::string(name) + suffix);
            if (offset >= 144u && offset <= 152u) {
                runtime.WriteWord(row + 164u + offset - 144u,
                                  std::bit_cast<std::uint32_t>(
                                      runtime.GetWidth(found)));
            }
        }
        const auto need_exp = runtime.ReadWord(row + 140u);
        const auto parent = runtime.ReadWord(need_exp + 68u);
        const auto position = runtime.GetPosition(need_exp, 0u);
        const auto absolute = runtime.WindowToScreen(parent, position);
        const auto relative = runtime.AbsoluteToRelative(
            runtime.GuiRootWindow(), absolute);
        runtime.WriteWord(row + 180u,
                          std::bit_cast<std::uint32_t>(relative.x));
        runtime.WriteWord(row + 184u,
                          std::bit_cast<std::uint32_t>(relative.y));
    }

    const auto callback = runtime.AllocateCallback(0x18u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 8u, kMouseDownCallback);
        runtime.WriteWord(callback + 12u, 0u);
        runtime.WriteWord(callback + 16u, dialog);
        runtime.WriteWord(callback, kCallbackVtable);
    }
    std::uint32_t connection = callback;
    try {
        connection = runtime.SubscribeMouseDown(
            runtime.ReadWord(dialog + 36u), callback);
    } catch (...) {
        if (connection != 0u) runtime.ReleaseCallback(connection);
        throw;
    }
    if (connection != 0u) runtime.ReleaseCallback(connection);
}

}  // namespace pal4
