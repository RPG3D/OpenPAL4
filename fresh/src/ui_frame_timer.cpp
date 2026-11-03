#include "pal4/ui_frame_timer.h"

namespace pal4 {

void UiFrameTimer::Add(
    const std::uint32_t queue, const std::uint32_t name,
    const std::uint32_t delay, const std::uint32_t owner,
    const std::uint32_t function, ITimedEventQueueRuntime& runtime) {
    TimedEventQueue::AddWithCallbackVtable(
        queue, name, delay, owner, function, kFirstCallbackVtable, runtime);
}

void UiFrameTimer::AddSecond(
    const std::uint32_t queue, const std::uint32_t name,
    const std::uint32_t delay, const std::uint32_t owner,
    const std::uint32_t function, ITimedEventQueueRuntime& runtime) {
    TimedEventQueue::AddWithCallbackVtable(
        queue, name, delay, owner, function, kSecondCallbackVtable, runtime);
}

std::uint32_t UiTimerCallback::Delete(
    const std::uint32_t callback, const std::uint8_t flags,
    IUiTimerCallbackRuntime& runtime) {
    Destruct(callback, runtime);
    if ((flags & 1u) != 0u) runtime.Free(callback);
    return callback;
}

void UiTimerCallback::Destruct(
    const std::uint32_t callback, IUiTimerCallbackRuntime& runtime) {
    runtime.WriteWord(callback, kBaseVtable);
}

}  // namespace pal4
