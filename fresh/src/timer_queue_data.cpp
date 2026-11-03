#include "pal4/timer_queue_data.h"

namespace pal4 {

void TimerQueueData::Copy(TimerQueueEntryState* destination,
                          TimerQueueEntryState& source,
                          ITimerQueueDataRuntime& runtime) {
    if (destination == nullptr) {
        return;
    }

    destination->text_flag = source.text_flag;
    destination->name = {};
    runtime.AssignName(*destination, source, 0u, 0xffffffffu);
    destination->initial_period_bits = source.initial_period_bits;
    destination->current_period_bits = source.current_period_bits;
    destination->owns_callback = source.owns_callback;
    destination->callback = source.callback;
    source.owns_callback = false;
}

TimerQueueEntryState& TimerQueueData::Assign(
    TimerQueueEntryState& destination, TimerQueueEntryState& source,
    const std::uint32_t empty_string, ITimerQueueDataRuntime& runtime) {
    destination.text_flag = source.text_flag;
    destination.name = {};
    const auto length = source.name.length;
    if (&destination == &source) {
        runtime.EraseName(destination, length, 0xffffffffu);
        runtime.EraseName(destination, 0u, 0u);
    } else {
        const auto source_data = source.name.data != 0u
                                     ? source.name.data : empty_string;
        const auto count = length != 0u
                               ? runtime.ReadReferenceCount(source_data)
                               : 0xffu;
        if (length != 0u && count < 0xfeu) {
            runtime.SetTextFlag(destination, 1u);
            destination.name = source.name;
            runtime.WriteReferenceCount(
                source_data, static_cast<std::uint8_t>(count + 1u));
        } else if (runtime.AllocateName(destination, length, 1u)) {
            runtime.CopyName(destination.name.data, source_data, length);
            runtime.SetNameSize(destination, length);
        }
    }
    destination.initial_period_bits = source.initial_period_bits;
    destination.current_period_bits = source.current_period_bits;
    destination.owns_callback = source.owns_callback;
    destination.callback = source.callback;
    source.owns_callback = false;
    return destination;
}

}  // namespace pal4
