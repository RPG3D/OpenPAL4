#include "pal4/smith_window.h"

#include <array>

namespace pal4 {
namespace {

std::uint32_t Window(const std::uint32_t object,
                     const std::uint32_t offset,
                     ISmithWindowVisibilityRuntime& runtime) {
    return runtime.ReadWord(object + offset);
}

void PublishVisibility(const std::uint32_t object,
                       const std::uint8_t visible,
                       const std::uint32_t key,
                       ISmithWindowVisibilityRuntime& runtime) {
    const auto root = Window(object, 0x24u, runtime);
    const auto tree = runtime.FrameManager() + 0x104u;
    runtime.ProcessUIEvent(tree, key, visible, root);
}

void RemoveAttachedFrames(const std::uint32_t object,
                          ISmithWindowVisibilityRuntime& runtime) {
    const auto root = Window(object, 0x24u, runtime);
    const auto toolbar_manager = runtime.FrameManager();
    const auto toolbar_name = runtime.ReadWord(0x8b1398u);
    const auto toolbar = runtime.FindFrame(toolbar_manager, toolbar_name);
    if (toolbar != 0u) {
        const auto toolbar_window = runtime.ReadWord(toolbar + 0x24u);
        if (runtime.ReadWord(toolbar_window + 0x44u) == root)
            runtime.RemoveChildWindow(root, toolbar_window);
    }
    const auto info_manager = runtime.FrameManager();
    const auto info_name = runtime.ReadWord(0x8b139cu);
    const auto info = runtime.FindFrame(info_manager, info_name);
    if (info != 0u &&
        runtime.ReadWord(runtime.ReadWord(info + 0x24u) + 0x44u) == root) {
        const auto toolbar_window = runtime.ReadWord(toolbar + 0x24u);
        runtime.RemoveChildWindow(
            runtime.ReadWord(toolbar_window + 0x44u), toolbar_window);
    }
}

}  // namespace

std::uint8_t SmithWindow::SetVisible(
    const std::uint32_t object, const std::uint8_t visible,
    const std::uint32_t parent,
    ISmithWindowVisibilityRuntime& runtime) {
    runtime.SetParent(object, visible, parent);
    PublishVisibility(object, visible, 0x8b1394u, runtime);
    PublishVisibility(object, visible, 0x8b13a0u, runtime);
    runtime.HandleEffect(object + 0xa4u, 1u);

    if (visible != 0u) {
        const auto first_active = runtime.SmithActive();
        runtime.SetSize(Window(object, 0x24u, runtime), 0x3f800000u,
                        first_active ? 0x3f666666u : 0x3f800000u);
        runtime.UpdateDisplay(object);
        if (runtime.SmithActive()) {
            for (const auto offset : {0x60u, 0x64u, 0x68u})
                runtime.SetEnabled(Window(object, offset, runtime), true);
        } else {
            constexpr std::array<std::uint32_t, 3> offsets{
                0x60u, 0x64u, 0x68u};
            for (std::uint32_t i = 0u; i < offsets.size(); ++i) {
                const auto enabled = runtime.GameDataEntry(16u + i) > 0;
                runtime.SetEnabled(Window(object, offsets[i], runtime),
                                   enabled);
            }
        }
        runtime.ProcessUIEvent(runtime.FrameManager() + 0x104u,
                               0x8b13a4u, 9u, 0u);
        const auto bar = Window(object, 0x74u, runtime);
        const auto active = runtime.SmithActive();
        if (bar != 0u) runtime.SetVisible(bar, !active);
        const auto adapter = object + 0x28u;
        (void)runtime.RemoveActiveFrame(runtime.FrameManager() + 0x134u,
                                        adapter);
        runtime.InsertActiveFrame(runtime.FrameManager() + 0x134u,
                                  adapter);
        runtime.SetVisible(Window(object, 0x7cu, runtime), false);
        runtime.SetAlwaysOnTop(Window(object, 0x44u, runtime), true);
        return runtime.OnTabChange(object, 0u);
    }

    if (runtime.IsDialogVisible(object)) {
        runtime.ResetState(object);
        RemoveAttachedFrames(object, runtime);
        runtime.SetSmithActive(false);
    }
    runtime.HandleEffect(object + 0xa4u, 1u);
    runtime.ClearModelPreview(object);
    return runtime.RemoveActiveFrame(runtime.FrameManager() + 0x134u,
                                     object + 0x28u);
}

}  // namespace pal4
