#include "pal4/menu_window_visibility.h"

#include <cstdint>

namespace pal4 {

void MenuWindowVisibility::Show(
    const std::uint32_t menu, const MenuWindowPoint screen,
    IMenuWindowVisibilityRuntime& runtime) {
    runtime.AdjustHeight(menu);
    runtime.SetParent(menu, 1u, 0u);
    const auto root = runtime.ReadWord(menu + 36u);
    runtime.SetAlwaysOnTop(root, true);
    const auto local = runtime.ScreenToWindow(root, screen);
    runtime.SetPosition(runtime.ReadWord(menu + 80u), local);
    runtime.WriteWord(menu + 60u, 0u);

    std::uint32_t index = 0u;
    while (true) {
        const auto begin = runtime.ReadWord(menu + 48u);
        if (begin == 0u) break;
        const auto end = runtime.ReadWord(menu + 52u);
        if (index >= (end - begin) / 4u) break;
        const auto window = runtime.ReadWord(begin + index * 4u);
        if (runtime.IsDisabled(window, false)) {
            runtime.WriteWord(menu + 60u,
                              runtime.ReadWord(menu + 60u) + 1u);
        }
        ++index;
    }
    runtime.SelectNavigation(menu + 40u, 12u);
    runtime.SetModalState(root, true);
}

void MenuWindowVisibility::HideAndCleanup(
    const std::uint32_t menu,
    IMenuWindowVisibilityRuntime& runtime) {
    runtime.WriteWord(menu + 52u, runtime.ReadWord(menu + 48u));
    runtime.WriteWord(menu + 60u, 0u);
    const auto begin = runtime.ReadWord(menu + 68u);
    const auto end = runtime.ReadWord(menu + 72u);
    for (auto record = begin; record != end; record += 164u)
        runtime.CleanupButtonRecord(record);
    runtime.WriteWord(menu + 72u, begin);

    const auto panel = runtime.ReadWord(menu + 80u);
    const auto child_begin = runtime.ReadWord(panel + 32u);
    const auto child_count = child_begin == 0u
        ? 0u : (runtime.ReadWord(panel + 36u) - child_begin) / 4u;
    const auto model = runtime.ReadWord(menu + 84u);
    for (std::uint32_t index = 0u; index < child_count; ++index) {
        const auto first = runtime.ReadWord(
            runtime.ReadWord(panel + 32u));
        runtime.RemoveChild(panel, first);
        if (first != model) runtime.DestroyWindow(first);
    }
    runtime.AddChild(panel, model);
    runtime.SetModalState(runtime.ReadWord(menu + 36u), false);
}

}  // namespace pal4
