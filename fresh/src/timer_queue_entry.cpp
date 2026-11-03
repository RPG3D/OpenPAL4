#include "pal4/timer_queue_entry.h"

namespace pal4 {

TimerQueueEntryState& TimerQueueEntry::Construct(
    TimerQueueEntryState& entry, const TimerQueueEntryState& source,
    const std::uint32_t period_bits, const std::uint32_t callback,
    const std::uint32_t empty_string,
    ITimerQueueEntryRuntime& runtime) {
    entry.text_flag = source.text_flag;
    entry.name = {};
    const auto length = source.name.length;
    if (&entry == &source) {
        runtime.EraseName(entry, length, 0xffffffffu);
        runtime.EraseName(entry, 0u, 0u);
    } else {
        const auto source_data = source.name.data != 0u
                                     ? source.name.data : empty_string;
        const auto count = length != 0u
                               ? runtime.ReadReferenceCount(source_data)
                               : 0xffu;
        if (length != 0u && count < 0xfeu) {
            runtime.SetTextFlag(entry, 1u);
            entry.name = source.name;
            runtime.WriteReferenceCount(source_data,
                                        static_cast<std::uint8_t>(count + 1u));
        } else if (runtime.AllocateName(entry, length, 1u)) {
            runtime.CopyName(entry.name.data, source_data, length);
            runtime.SetNameSize(entry, length);
        }
    }
    entry.initial_period_bits = period_bits;
    entry.current_period_bits = period_bits;
    entry.callback = callback;
    entry.owns_callback = callback != 0u;
    return entry;
}

std::uint32_t TimerQueueEntry::Destroy(
    TimerQueueEntryState& entry, ITimerQueueEntryRuntime& runtime) {
    if (entry.owns_callback && entry.callback != 0u) {
        runtime.ReleaseCallback(entry.callback, 1u);
    }
    std::uint32_t result = 0u;
    if (entry.name.data != 0u) {
        const auto count = runtime.ReadReferenceCount(entry.name.data);
        const auto header = entry.name.data - 1u;
        if (count == 0u || count == 0xffu) {
            result = runtime.FreeName(header);
        } else {
            runtime.WriteReferenceCount(entry.name.data,
                                        static_cast<std::uint8_t>(count - 1u));
            result = header;
        }
    }
    entry.name = {};
    return result;
}

std::uint32_t TimerQueueEntry::DestroyAndMaybeFree(
    const std::uint32_t address, const std::uint8_t deleting,
    ITimerQueueEntryDeletingRuntime& runtime) {
    (void)Destroy(runtime.EntryAt(address), runtime);
    if ((deleting & 1u) != 0u) runtime.FreeObject(address);
    return address;
}

}  // namespace pal4
