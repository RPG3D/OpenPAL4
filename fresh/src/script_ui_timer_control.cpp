#include "pal4/script_ui_timer_control.h"

namespace pal4 {

void ScriptUiTimerControl::Clear(IScriptUiTimerControlRuntime& runtime) {
    const auto frame = runtime.FindFrame(runtime.FrameManager(), "portrait");
    if (frame != 0u) runtime.ClearTimerAndText(frame);
}

std::uint32_t ScriptUiTimerControl::Pause(
    IScriptUiTimerControlRuntime& runtime) {
    const auto frame = runtime.FindFrame(runtime.FrameManager(), "portrait");
    if (frame == 0u) return 0u;
    const auto queue = runtime.ReadWord(runtime.FrameManager() + 0x1a0u);
    return runtime.RemoveTask(queue, frame + 0xd0u);
}

void ScriptUiTimerControl::Resume(IScriptUiTimerControlRuntime& runtime) {
    const auto frame = runtime.FindFrame(runtime.FrameManager(), "portrait");
    if (frame == 0u || runtime.ReadWord(frame + 0xe8u) == 0u) return;
    const auto queue = runtime.ReadWord(runtime.FrameManager() + 0x1a0u);
    runtime.CreateTask(queue, frame + 0xd0u, 0x3f800000u,
                       frame, 0x47caa0u, 0u);
}

}  // namespace pal4
