#include "pal4/ui_frame_manager_singleton.h"

#include <bit>

namespace pal4 {

void UiFrameManagerSingleton::ToggleEdgeWindows(
    const std::uint32_t manager, const bool enabled,
    IUiFrameManagerEdgeWindowsRuntime& runtime) {
    if (runtime.ReadConfigWord(0u) != 1280u ||
        runtime.ReadConfigWord(4u) != 800u) return;

    const auto first = runtime.ReadWord(manager + 0x1d4u);
    if (!enabled) {
        if (first != 0u) {
            const auto parent = runtime.ReadWord(first + 0x44u);
            if (parent != 0u) runtime.RemoveChildWindow(parent, first);
        }
        const auto second = runtime.ReadWord(manager + 0x1d8u);
        if (second != 0u) {
            const auto parent = runtime.ReadWord(second + 0x44u);
            if (parent != 0u) runtime.RemoveChildWindow(parent, second);
        }
        return;
    }

    const auto attach = [&](const std::uint32_t offset,
                            const float x) {
        const auto system = runtime.ReadWord(manager + 0x198u);
        const auto position = runtime.TransformMouseCoordinates(system, {x, 0.0f});
        const auto singleton = runtime.SystemSingleton();
        const auto child = runtime.ReadWord(manager + offset);
        const auto root = runtime.ReadWord(singleton + 0x30u);
        runtime.AddChildWindow(root, child);
        runtime.SetAlwaysOnTop(runtime.ReadWord(manager + offset), true);
        runtime.SetClippedByParent(runtime.ReadWord(manager + offset), false);
        runtime.SetPosition(runtime.ReadWord(manager + offset), 1u, position);
        runtime.SetWidth(runtime.ReadWord(manager + offset), 1u, 0x42a00002u);
        runtime.SetHeight(runtime.ReadWord(manager + offset), 1u, 0x44160000u);
    };

    if (first != 0u) attach(0x1d4u, 0.0f);
    if (runtime.ReadWord(manager + 0x1d8u) != 0u)
        attach(0x1d8u, std::bit_cast<float>(0x4492aaaau));
}

}  // namespace pal4
