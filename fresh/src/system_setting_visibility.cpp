#include "pal4/system_setting_visibility.h"

namespace pal4 {
namespace {

void SendEvent(ISystemSettingVisibilityRuntime& runtime,
               const std::uint32_t key, const std::uint32_t first,
               const std::uint32_t second) {
    runtime.ProcessUiEvent(runtime.FrameManager() + 0x104u,
                           key, first, second);
}

}  // namespace

void SystemSettingVisibility::SetVisible(
    const std::uint32_t frame, const std::uint8_t visible,
    const std::uint32_t parent,
    ISystemSettingVisibilityRuntime& runtime) {
    runtime.SetParent(frame, visible, parent);
    SendEvent(runtime, kShowFrameTool, visible, 0u);
    SendEvent(runtime, kShowGameInfo, visible,
              runtime.ReadWord(frame + 0x24u));
    runtime.ShowSystemFrame(frame);

    if (visible == 0u) {
        runtime.SaveSettings(runtime.InitializeGlobalData());
        runtime.RemoveActive(runtime.FrameManager() + 0x134u,
                             frame + 0x28u);
        return;
    }

    SendEvent(runtime, kHelpTips, 13u, 0u);
    runtime.UpdateStats(frame);
    runtime.InsertActive(runtime.FrameManager() + 0x134u,
                         frame + 0x28u);
    const auto window = runtime.ReadWord(frame + 0xb8u);
    runtime.SetEnabled(window, true);
    const auto arena = runtime.ReadWord(runtime.ArenaManager() + 0x108u);
    const auto load_data = arena != 0u &&
                           runtime.ReadWord(arena + 0x34u) == 2u;
    if (load_data) {
        runtime.SetEnabled(window, false);
    }
    const auto event = runtime.CreateEventArgs();
    try {
        if (load_data) {
            runtime.InvokeLoadData(frame, event);
        } else {
            runtime.InvokeStartNewGame(frame, event);
        }
    } catch (...) {
        runtime.DestroyEventArgs(event);
        throw;
    }
    runtime.DestroyEventArgs(event);
    runtime.UpdateSystemDialog(runtime.FrameManager(), 7u);
}

}  // namespace pal4
