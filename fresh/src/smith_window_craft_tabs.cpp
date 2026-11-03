#include "pal4/smith_window.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {
namespace {

struct CraftTab {
    std::string_view image;
    std::uint32_t event;
    std::uint32_t dialog_key;
    std::uint32_t mode;
};

constexpr std::array<CraftTab, 3> kCraftTabs{{
    {"rongzhu", 9u, 5u, 0u},
    {"duanye", 10u, 15u, 1u},
    {"zhuling", 11u, 16u, 2u},
}};

std::uint8_t HandleCraftTab(const std::uint32_t object,
                            const CraftTab& tab,
                            ISmithWindowCraftTabRuntime& runtime) {
    runtime.SetImage(runtime.ReadWord(object + 0x78u), "zi0", tab.image);
    runtime.ProcessUIEvent(0x8b13a4u, tab.event);
    runtime.UpdateSystemDialogVisibility(tab.dialog_key);
    runtime.ShowEffectPanel(object);
    runtime.HideSelectionPanel(object);
    return runtime.OnCraftModeClick(object, tab.mode);
}

}  // namespace

std::uint8_t SmithWindow::OnRongzhuTabClick(
    const std::uint32_t object, const std::uint32_t,
    ISmithWindowCraftTabRuntime& runtime) {
    return HandleCraftTab(object, kCraftTabs[0], runtime);
}

std::uint8_t SmithWindow::OnDuanyeTabClick(
    const std::uint32_t object, const std::uint32_t,
    ISmithWindowCraftTabRuntime& runtime) {
    return HandleCraftTab(object, kCraftTabs[1], runtime);
}

std::uint8_t SmithWindow::OnZhulingTabClick(
    const std::uint32_t object, const std::uint32_t,
    ISmithWindowCraftTabRuntime& runtime) {
    return HandleCraftTab(object, kCraftTabs[2], runtime);
}

std::uint8_t SmithWindow::DispatchCraftTabEvent(
    const std::uint32_t adapter, const std::uint32_t event,
    ISmithWindowCraftTabRuntime& runtime) {
    const auto object = adapter - 0x28u;
    switch (runtime.ReadWord(event + 0x10u)) {
        case 2u:
            (void)OnRongzhuTabClick(object, 0u, runtime);
            return 1u;
        case 3u:
            (void)OnDuanyeTabClick(object, 0u, runtime);
            return 1u;
        case 4u:
            (void)OnZhulingTabClick(object, 0u, runtime);
            return 1u;
        default:
            return 0u;
    }
}

}  // namespace pal4
