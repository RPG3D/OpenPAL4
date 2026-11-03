#include "pal4/system_setting_key_handler.h"

namespace pal4 {

std::uint8_t SystemSettingKeyHandler::OnExit(
    const std::uint32_t member, const std::uint32_t event,
    ISystemSettingKeyRuntime& runtime) {
    if (runtime.ReadWord(event + 16u) != 1u) {
        return runtime.ForwardKey(member, event);
    }
    const auto frame = member - 40u;
    runtime.HideFrame(frame, 0u, 0u);
    runtime.ProcessUiEvent(runtime.FrameManager() + 260u,
                           0x8a80acu, 1u, 0u);
    return 1u;
}

std::uint8_t SystemSettingKeyHandler::OnGameMenuKey(
    const std::uint32_t member, const std::uint32_t event,
    ISystemSettingKeyRuntime& runtime) {
    const auto code = runtime.ReadWord(event + 16u);
    const auto frame = member - 40u;
    switch (code) {
    case 2u:
        runtime.HandleMouseEvent(runtime.ReadWord(member + 144u));
        return 1u;
    case 3u:
        runtime.SelectLoadData(frame);
        return 1u;
    case 4u:
        runtime.ShowSystemSettings(frame);
        return 1u;
    case 5u:
        runtime.ConfirmQuit(frame);
        return 1u;
    default:
        return 0u;
    }
}

}  // namespace pal4
