#include "pal4/event_handler_array.h"

#include <new>

namespace pal4 {

std::uint32_t EventHandlerArray::Insert(
    const std::uint32_t array, const std::uint32_t handler,
    IEventHandlerArrayRuntime& runtime) {
    const auto end = runtime.ReadWord(array + 8u);
    if (runtime.ReadWord(array + 0xcu) != end) {
        runtime.WriteWord(end, handler);
        runtime.WriteWord(array + 8u, end + 4u);
        return runtime.InvokeVirtual(handler, 0xcu);
    }

    const auto begin = runtime.ReadWord(array + 4u);
    const auto size = begin != 0u ? (end - begin) / 4u : 0u;
    const auto growth = size > 1u ? size : 1u;
    const auto capacity = size + growth;
    const auto replacement = runtime.Allocate(capacity * 4u);
    if (replacement == 0u) throw std::bad_alloc();
    for (std::uint32_t i = 0; i < size; ++i)
        runtime.WriteWord(replacement + 4u * i,
                          runtime.ReadWord(begin + 4u * i));
    runtime.WriteWord(replacement + 4u * size, handler);
    runtime.Free(begin);
    runtime.WriteWord(array + 0xcu, replacement + 4u * capacity);
    runtime.WriteWord(array + 4u, replacement);
    runtime.WriteWord(array + 8u, replacement + 4u * (size + 1u));
    return runtime.InvokeVirtual(handler, 0xcu);
}

std::uint32_t EventHandlerArray::Remove(
    const std::uint32_t array, const std::uint32_t handler,
    IEventHandlerArrayRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4u);
    const auto old_end = runtime.ReadWord(array + 8u);
    auto result = begin;
    while (result != old_end) {
        if (runtime.ReadWord(result) == handler) {
            runtime.WriteWord(array + 8u, result);
            result = old_end;
            break;
        }
        result += 4u;
    }
    if (begin != 0u) {
        const auto remaining = (runtime.ReadWord(array + 8u) - begin) / 4u;
        if (remaining != 0u) {
            const auto last = runtime.ReadWord(begin + 4u * (remaining - 1u));
            return runtime.InvokeVirtual(last, 0x10u);
        }
        return remaining;
    }
    return result;
}

bool EventHandlerArray::Contains(
    const std::uint32_t array, const std::uint32_t event,
    IEventHandlerArrayRuntime& runtime) {
    const auto initial_begin = runtime.ReadWord(array + 4u);
    if (initial_begin != 0u) {
        const auto initial_end = runtime.ReadWord(array + 8u);
        if (initial_end != initial_begin &&
            runtime.InvokePredicate(runtime.ReadWord(initial_end - 4u),
                                    4u, event) != 0u)
            return true;
    }

    const auto begin = runtime.ReadWord(array + 4u);
    if (begin == 0u) return false;
    const auto end = runtime.ReadWord(array + 8u);
    for (auto cursor = end; cursor != begin;) {
        cursor -= 4u;
        if (runtime.InvokePredicate(runtime.ReadWord(cursor), 8u,
                                    event) != 0u)
            return true;
    }
    return false;
}

std::uint32_t EventHandlerArray::GetLast(
    const std::uint32_t array, IEventHandlerArrayRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4u);
    if (begin == 0u) return 0u;
    const auto end = runtime.ReadWord(array + 8u);
    if (end == begin) return 0u;
    return runtime.ReadWord(end - 4u);
}

std::uint32_t EventHandlerArray::ForEach(
    const std::uint32_t array, IEventHandlerArrayRuntime& runtime) {
    auto cursor = runtime.ReadWord(array + 4u);
    auto end = runtime.ReadWord(array + 8u);
    while (cursor != end) {
        (void)runtime.InvokeVirtual(runtime.ReadWord(cursor), 0x14u);
        cursor += 4u;
        end = runtime.ReadWord(array + 8u);
    }
    return end;
}

}  // namespace pal4
