#include "pal4/timer_queue_task.h"

namespace pal4 {

std::uint32_t TimerCallbackInvocation::InvokeWithOffset(
    const std::uint32_t callback, ITimerCallbackInvocationRuntime& runtime) {
    const auto offset = runtime.ReadWord(callback + 0xcu);
    const auto base = runtime.ReadWord(callback + 4u);
    const auto function = runtime.ReadWord(callback + 8u);
    return runtime.InvokeThiscall(function, base + offset);
}
namespace {

void InsertEntry(
    const std::uint32_t queue, const std::uint32_t name_source,
    const std::uint32_t period_bits, const std::uint32_t callback,
    ITimerQueueTaskRuntime& runtime) {
    TimerQueueEntryState entry{};
    entry.owns_callback = callback != 0u;
    entry.callback = callback;
    try {
        runtime.CopyName(entry, name_source);
    } catch (...) {
        (void)TimerQueueEntry::Destroy(entry, runtime.EntryRuntime());
        throw;
    }
    entry.initial_period_bits = period_bits;
    entry.current_period_bits = period_bits;

    try {
        const auto predecessor = runtime.ReadWord(queue + 4u);
        const auto successor = runtime.ReadWord(predecessor + 4u);
        const auto node = runtime.Allocate(0x28u);
        if (node == 0u) {
            (void)TimerQueueEntry::Destroy(entry, runtime.EntryRuntime());
            return;
        }
        runtime.WriteWord(node, predecessor);
        runtime.WriteWord(node + 4u, successor != 0u ? successor : node);
        runtime.WriteWord(predecessor + 4u, node);
        runtime.WriteWord(runtime.ReadWord(node + 4u), node);
        runtime.AssignTimerData(node + 8u, entry);
        runtime.WriteWord(queue + 8u,
                          runtime.ReadWord(queue + 8u) + 1u);
    } catch (...) {
        (void)TimerQueueEntry::Destroy(entry, runtime.EntryRuntime());
        throw;
    }

    if (entry.owns_callback && entry.callback != 0u) {
        runtime.ReleaseCallback(entry.callback, 1u);
        entry.owns_callback = false;
    }
    (void)TimerQueueEntry::Destroy(entry, runtime.EntryRuntime());
}

void CreateWithVtable(
    const std::uint32_t queue, const std::uint32_t name_source,
    const std::uint32_t period_bits, const std::uint32_t callback_owner,
    const std::uint32_t callback_function,
    const std::uint32_t callback_argument,
    const std::uint32_t callback_vtable, ITimerQueueTaskRuntime& runtime) {
    if (callback_function == 0u) return;
    const auto callback = runtime.Allocate(0x10u);
    if (callback != 0u) {
        runtime.WriteWord(callback, callback_vtable);
        runtime.WriteWord(callback + 4u, callback_owner);
        runtime.WriteWord(callback + 8u, callback_function);
        runtime.WriteWord(callback + 0xcu, callback_argument);
    }
    InsertEntry(queue, name_source, period_bits, callback, runtime);
}

}  // namespace

void TimerQueueTask::Create(
    const std::uint32_t queue, const std::uint32_t name_source,
    const std::uint32_t period_bits, const std::uint32_t callback_owner,
    const std::uint32_t callback_function,
    const std::uint32_t callback_argument,
    ITimerQueueTaskRuntime& runtime) {
    CreateWithVtable(queue, name_source, period_bits, callback_owner,
                     callback_function, callback_argument,
                     kCallbackVtable, runtime);
}

void TimerQueueTask::SchedulePuzzle(
    const std::uint32_t queue, const std::uint32_t name_source,
    const std::uint32_t period_bits, const std::uint32_t callback_owner,
    const std::uint32_t callback_function,
    const std::uint32_t callback_argument,
    ITimerQueueTaskRuntime& runtime) {
    CreateWithVtable(queue, name_source, period_bits, callback_owner,
                     callback_function, callback_argument,
                     kPuzzleCallbackVtable, runtime);
}

void TimerQueueTask::InsertOwnedCallback(
    const std::uint32_t queue, const std::uint32_t name_source,
    const std::uint32_t period_bits, const std::uint32_t callback,
    ITimerQueueTaskRuntime& runtime) {
    InsertEntry(queue, name_source, period_bits, callback, runtime);
}

}  // namespace pal4
