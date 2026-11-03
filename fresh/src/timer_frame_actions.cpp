#include "pal4/timer_frame_actions.h"

namespace pal4 {
namespace {

std::uint8_t ChooseAndClose(const std::uint32_t frame,
                            const std::uint32_t choice,
                            ITimerFrameActionsRuntime& runtime) {
    runtime.WriteWord(frame + 0x70u, choice);
    runtime.SetFrameVisibility(frame, 0u, 0u);
    return 1u;
}

}  // namespace

std::uint8_t TimerFrameActions::OnButtonKey(
    const std::uint32_t frame, const std::uint32_t event,
    ITimerFrameActionsRuntime& runtime) {
    if (runtime.ReadWord(event + 0x10u) == 1u) {
        return ChooseAndClose(frame, 0u, runtime);
    }
    runtime.ForwardMemberEvent(frame + 0x28u, event);
    return 1u;
}

std::uint8_t TimerFrameActions::OnSettle(
    const std::uint32_t frame, ITimerFrameActionsRuntime& runtime) {
    return ChooseAndClose(frame, 1u, runtime);
}

std::uint8_t TimerFrameActions::OnDelegate(
    const std::uint32_t frame, ITimerFrameActionsRuntime& runtime) {
    return ChooseAndClose(frame, 2u, runtime);
}

std::uint8_t TimerFrameActions::OnExit(
    const std::uint32_t frame, ITimerFrameActionsRuntime& runtime) {
    return ChooseAndClose(frame, 0u, runtime);
}

TimerCallbackState& UiFrameEventCallback::Construct(
    TimerCallbackState& callback, const std::uint32_t function,
    const std::uint32_t context, const std::uint32_t owner) {
    callback.owner = function;
    callback.function = context;
    callback.argument = owner;
    callback.vtable = kVtable;
    return callback;
}

}  // namespace pal4
