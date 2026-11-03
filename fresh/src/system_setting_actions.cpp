#include "pal4/system_setting_actions.h"

namespace pal4 {
namespace {

std::uint32_t SelectMainButton(const std::uint32_t frame,
                               const std::uint32_t button_offset,
                               const bool reset_visual,
                               ISystemSettingSelectionRuntime& runtime) {
    const auto button = runtime.ReadWord(frame + button_offset);
    runtime.SetFocusedButton(frame + 40u, button);
    const auto begin = runtime.ReadWord(frame + 48u);
    const auto end = runtime.ReadWord(frame + 52u);
    for (auto current = begin; current != end; current += 4u) {
        const auto active = runtime.ReadWord(current);
        runtime.SetSelected(active, false);
        if (reset_visual) {
            runtime.WriteByte(active + 1201u, 0u);
            runtime.RequestRedraw(active);
        }
    }
    return button;
}

}  // namespace

std::uint8_t SystemSettingActions::OnLoadData(
    const std::uint32_t frame, const std::uint32_t event,
    ISystemSettingActionsRuntime& runtime) {
    (void)event;
    SelectLoadData(frame, runtime);
    return 1u;
}

void SystemSettingActions::SelectLoadData(
    const std::uint32_t frame, ISystemSettingSelectionRuntime& runtime) {
    runtime.WriteWord(frame + 504u, 2u);
    runtime.ShowDataMode(frame);
    const auto button = SelectMainButton(frame, 188u, false, runtime);
    runtime.SetSelected(button, true);
    const auto image = runtime.ReadWord(frame + 180u);
    runtime.SetVisible(image, true);
    runtime.SetImage(image, "zi0", "duquyouxi");
    runtime.WriteWord(frame + 60u, 1u);
}

std::uint8_t SystemSettingActions::OnStartNewGame(
    const std::uint32_t frame, const std::uint32_t event,
    ISystemSettingActionsRuntime& runtime) {
    (void)event;
    SelectStartNewGame(frame, runtime);
    return 1u;
}

void SystemSettingActions::SelectStartNewGame(
    const std::uint32_t frame, ISystemSettingSelectionRuntime& runtime) {
    runtime.WriteWord(frame + 504u, 1u);
    runtime.ShowDataMode(frame);
    const auto button = SelectMainButton(frame, 184u, false, runtime);
    constexpr std::uint32_t groups[][2] = {
        {200u, 3u}, {212u, 2u}, {220u, 3u}, {232u, 2u}};
    for (const auto& group : groups) {
        for (std::uint32_t index = 0; index < group[1]; ++index) {
            const auto control = runtime.ReadWord(frame + group[0] + 4u * index);
            runtime.WriteByte(control + 1201u, 0u);
            runtime.RequestRedraw(control);
        }
    }
    runtime.SetSelected(button, true);
    const auto image = runtime.ReadWord(frame + 180u);
    runtime.SetVisible(image, true);
    runtime.SetImage(image, "zi0", "chucunyouxi");
    runtime.WriteWord(frame + 60u, 0u);
}

std::uint8_t SystemSettingActions::OnSystemSettings(
    const std::uint32_t frame, const std::uint32_t event,
    ISystemSettingActionsRuntime& runtime) {
    (void)event;
    SelectSystemSettings(frame, runtime);
    return 1u;
}

void SystemSettingActions::SelectSystemSettings(
    const std::uint32_t frame, ISystemSettingSelectionRuntime& runtime) {
    runtime.WriteWord(frame + 504u, 0u);
    runtime.ShowSave(frame);
    const auto button = SelectMainButton(frame, 192u, true, runtime);
    runtime.SetSelected(button, true);
    const auto image = runtime.ReadWord(frame + 180u);
    runtime.SetVisible(image, true);
    runtime.SetImage(image, "zi0", "xitongsheding");
    runtime.WriteWord(frame + 60u, 2u);
}

std::uint8_t SystemSettingActions::OnQuitGame(
    const std::uint32_t frame, const std::uint32_t event,
    ISystemSettingActionsRuntime& runtime) {
    (void)event;
    (void)ConfirmQuit(frame, runtime);
    return 1u;
}

std::uint32_t SystemSettingActions::ConfirmQuit(
    const std::uint32_t frame, ISystemSettingActionsRuntime& runtime) {
    const auto button = SelectMainButton(frame, 196u, false, runtime);
    runtime.SetSelected(button, true);
    runtime.WriteWord(frame + 60u, 3u);
    const auto callback = runtime.Allocate(0x18u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 8u, 0x481630u);
        runtime.WriteWord(callback + 12u, 0u);
        runtime.WriteWord(callback + 16u, frame);
        runtime.WriteWord(callback, 0x843c08u);
    }
    const auto text = runtime.TriggerText(217u);
    const auto manager = runtime.FrameManager();
    runtime.CreateCommonDialog(manager);
    std::uint32_t result = 0u;
    try {
        result = runtime.ShowDialog(text, 1050253722u,
                                    1053609165u, 3u, callback);
    } catch (...) {
        if (callback != 0u) {
            (void)runtime.ReleaseCallback(callback);
        }
        throw;
    }
    return callback != 0u ? runtime.ReleaseCallback(callback) : result;
}

std::uint8_t SystemSettingActions::OnQuitConfirmed(
    const std::uint32_t frame, const std::uint32_t event,
    ISystemSettingActionsRuntime& runtime) {
    if (event == 1u) {
        runtime.ResetArena(runtime.ArenaManager());
        runtime.ProcessUiEvent(runtime.FrameManager() + 260u,
                               0x8a80b4u, 0u, 0u);
        runtime.InitializeGameState(runtime.Game(), 0u, 0u);
    } else {
        const auto button = runtime.ReadWord(frame + 196u);
        runtime.SetSelected(button, false);
        runtime.SetFocusedButton(frame + 40u, button);
    }
    return 1u;
}

}  // namespace pal4
