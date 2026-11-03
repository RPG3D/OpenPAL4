#include "pal4/timer_frame_buttons.h"

namespace pal4 {
namespace {

struct ButtonSpec {
    std::string_view suffix;
    std::uint32_t window_offset;
    std::uint32_t position_offset;
    std::uint32_t trigger_id;
    std::uint32_t callback;
    bool constructor_callback;
};

std::uint32_t MakeCallback(const std::uint32_t frame,
                           const ButtonSpec& spec,
                           ITimerFrameButtonsRuntime& runtime) {
    const auto allocation = runtime.Allocate(0x18u);
    if (allocation == 0u) {
        return 0u;
    }
    if (spec.constructor_callback) {
        try {
            return runtime.ConstructCallback(allocation, spec.callback,
                                             frame);
        } catch (...) {
            runtime.Free(allocation);
            throw;
        }
    }
    runtime.WriteWord(allocation + 8u, spec.callback);
    runtime.WriteWord(allocation + 0xcu, 0u);
    runtime.WriteWord(allocation + 0x10u, frame);
    runtime.WriteWord(allocation, TimerFrameButtons::kCallbackVtable);
    return allocation;
}

void BindButton(const std::uint32_t frame,
                const std::uint32_t name_buffer,
                const ButtonSpec& spec,
                ITimerFrameButtonsRuntime& runtime) {
    const auto name = runtime.ResetName(name_buffer, frame, spec.suffix);
    const auto window = runtime.FindWindow(name);
    runtime.WriteWord(frame + spec.window_offset, window);
    const auto callback = MakeCallback(frame, spec, runtime);
    const auto event = runtime.ReadWord(TimerFrameButtons::kClickedEvent);
    const auto connection = runtime.Subscribe(window, event, callback);
    if (connection != 0u) {
        runtime.ReleaseConnection(connection);
    }
    auto text = runtime.TriggerText(spec.trigger_id);
    if (text == 0u) {
        text = TimerFrameButtons::kEmptyText;
    }
    runtime.SetText(window, runtime.ToUtf8(text));
    const auto position = runtime.Position(window);
    runtime.WriteWord(frame + spec.position_offset, position[0]);
    runtime.WriteWord(frame + spec.position_offset + 4u, position[1]);
}

}  // namespace

std::uint32_t TimerFrameButtons::Initialize(
    const std::uint32_t frame, ITimerFrameButtonsRuntime& runtime) {
    const auto name_buffer = runtime.BeginName(frame);
    try {
        constexpr ButtonSpec buttons[] = {
            {"BtnSettle", 0x58u, 0x40u, 278u, kSettleClick, true},
            {"BtnDelegate", 0x5cu, 0x48u, 189u, kDelegateClick, true},
            {"BtnExit", 0x60u, 0x50u, 30u, kExitClick, false},
        };
        for (const auto& button : buttons) {
            BindButton(frame, name_buffer, button, runtime);
        }

        const auto panel_name = runtime.ResetName(
            name_buffer, frame, "PanelButton");
        const auto panel = runtime.FindWindow(panel_name);
        runtime.WriteWord(frame + 0x68u, panel);
        runtime.WriteWord(frame + 0x6cu, runtime.HeightBits(panel));

        const auto image_name = runtime.ResetName(
            name_buffer, frame, "imgBottom");
        runtime.WriteWord(frame + 0x64u, runtime.FindWindow(image_name));

        const ButtonSpec key_spec{"", 0u, 0u, 0u, kKeyDown, false};
        const auto callback = MakeCallback(frame, key_spec, runtime);
        const auto root = runtime.ReadWord(frame + 0x24u);
        const auto event = runtime.ReadWord(kKeyDownEvent);
        const auto connection = runtime.Subscribe(root, event, callback);
        if (connection != 0u) {
            runtime.ReleaseConnection(connection);
        }
    } catch (...) {
        (void)runtime.EndName(name_buffer);
        throw;
    }
    return runtime.EndName(name_buffer);
}

}  // namespace pal4
