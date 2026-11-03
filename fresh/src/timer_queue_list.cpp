#include "pal4/timer_queue_list.h"

namespace pal4 {

std::uint32_t TimerQueueList::Destroy(
    const std::uint32_t list, ITimerQueueListRuntime& runtime) {
    const auto head = runtime.ReadWord(list + 4u);
    auto node = runtime.ReadWord(head);
    while (node != head) {
        const auto current = node;
        node = runtime.ReadWord(current);
        const auto previous = runtime.ReadWord(current + 4u);
        const auto next = runtime.ReadWord(current);
        runtime.WriteWord(previous, next);
        const auto next_again = runtime.ReadWord(current);
        const auto previous_again = runtime.ReadWord(current + 4u);
        runtime.WriteWord(next_again + 4u, previous_again);
        runtime.DestroyEntry(current + 8u);
        runtime.Free(current);
        runtime.WriteWord(list + 8u, runtime.ReadWord(list + 8u) - 1u);
    }
    runtime.Free(runtime.ReadWord(list + 4u));
    runtime.WriteWord(list + 4u, 0u);
    runtime.WriteWord(list + 8u, 0u);
    return 0u;
}

}  // namespace pal4
