#include "pal4/jigsaw_window_step.h"

namespace pal4 {
namespace {

void ReleaseStepName(const StringBufferState& name,
                     ITimerQueueEntryRuntime& runtime) {
    if (name.data == 0u) return;
    const auto count = runtime.ReadReferenceCount(name.data);
    if (count == 0u || count == 0xffu)
        (void)runtime.FreeName(name.data - 1u);
    else
        runtime.WriteReferenceCount(
            name.data, static_cast<std::uint8_t>(count - 1u));
}

}  // namespace

std::uint32_t JigsawWindowStep::SetWithTimer(
    const std::uint32_t frame, const std::uint32_t step,
    IJigsawWindowStepRuntime& runtime) {
    runtime.ProcessPuzzleEvent(frame + 0x2cu, step + 6u);
    if (runtime.ReadByte(frame + 0x78u) != 0u)
        return runtime.UpdateCounter(frame);

    TimerQueueEntryState source{};
    source.text_flag = static_cast<std::uint8_t>(step);
    source.name = runtime.BuildStepName(source.text_flag, "PuzzleStep");
    auto& entry_runtime = runtime.EntryRuntime();
    TimerQueueEntryState entry{};
    std::uint32_t callback = 0u;
    bool entry_ready = false;
    try {
        const auto queue = runtime.ReadWord(runtime.FrameManager() + 0x1a0u);
        callback = runtime.Allocate(0x10u);
        if (callback != 0u) {
            runtime.WriteWord(callback + 8u, kDecrementFunction);
            runtime.WriteWord(callback, kCallbackVtable);
            runtime.WriteWord(callback + 4u, frame);
            runtime.WriteWord(callback + 0xcu, 0u);
        }
        runtime.CopyFloorAttribute(entry, source);
        entry_ready = true;
        entry.initial_period_bits = 0x3f800000u;
        entry.current_period_bits = 0x3f800000u;
        entry.owns_callback = callback != 0u;
        entry.callback = callback;
        runtime.InsertTimer(queue, step,
                            runtime.ReadWord(queue + 4u), entry);
    } catch (...) {
        if (entry_ready)
            (void)TimerQueueEntry::Destroy(entry, entry_runtime);
        else {
            if (entry.name.data != 0u)
                (void)TimerQueueEntry::Destroy(entry, entry_runtime);
            if (callback != 0u)
                entry_runtime.ReleaseCallback(callback, 1u);
        }
        ReleaseStepName(source.name, entry_runtime);
        throw;
    }
    (void)TimerQueueEntry::Destroy(entry, entry_runtime);
    ReleaseStepName(source.name, entry_runtime);
    return runtime.UpdateCounter(frame);
}

}  // namespace pal4
