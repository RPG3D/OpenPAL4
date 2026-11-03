#include "pal4/timed_event_queue.h"

#include <new>

namespace pal4 {
namespace {

void InsertRecord(const std::uint32_t queue, TimedQueueRecord& temporary,
                  ITimedEventQueueRuntime& runtime) {
    const auto head = runtime.ReadWord(queue + 0x10u);
    const auto successor = runtime.ReadWord(head + 4u);
    const auto node = runtime.Allocate(0x24u);
    if (node == 0u) throw std::bad_alloc();
    runtime.WriteWord(node, head);
    runtime.WriteWord(node + 4u, successor != 0u ? successor : node);
    runtime.WriteWord(head + 4u, node);
    runtime.WriteWord(runtime.ReadWord(node + 4u), node);
    runtime.CopyRecord(node + 8u, temporary);
    runtime.WriteWord(queue + 0x14u,
                      runtime.ReadWord(queue + 0x14u) + 1u);
}

}  // namespace

void TimedEventQueue::Add(const std::uint32_t queue,
                          const std::uint32_t name,
                          const std::uint32_t delay,
                          const std::uint32_t callback_owner,
                          const std::uint32_t callback_function,
                          ITimedEventQueueRuntime& runtime) {
    AddWithCallbackVtable(queue, name, delay, callback_owner,
                          callback_function, kCallbackVtable, runtime);
}

void TimedEventQueue::AddWithCallbackVtable(
    const std::uint32_t queue, const std::uint32_t name,
    const std::uint32_t delay, const std::uint32_t callback_owner,
    const std::uint32_t callback_function,
    const std::uint32_t callback_vtable,
    ITimedEventQueueRuntime& runtime) {
    if (callback_function == 0u) return;

    const auto callback = runtime.Allocate(0xcu);
    if (callback != 0u) {
        runtime.WriteWord(callback + 8u, callback_function);
        runtime.WriteWord(callback, callback_vtable);
        runtime.WriteWord(callback + 4u, callback_owner);
    }

    TimedQueueRecord temporary{};
    runtime.InitializeName(temporary, name);
    temporary.callback = callback;
    temporary.delay = delay;
    temporary.owns_callback = callback != 0u;

    InsertRecord(queue, temporary, runtime);

    if (temporary.owns_callback != 0u && temporary.callback != 0u)
        runtime.DeleteCallback(temporary.callback, 1u);
    if (temporary.name_data != 0u)
        runtime.ReleaseName(temporary);
}

std::uint32_t TimedEventQueue::AddExistingCallback(
    const std::uint32_t queue, const std::uint32_t name,
    const std::uint32_t delay, const std::uint32_t callback,
    ITimedEventQueueRuntime& runtime) {
    TimedQueueRecord temporary{};
    runtime.InitializeName(temporary, name);
    temporary.delay = delay;
    temporary.owns_callback = callback != 0u;
    temporary.callback = callback;
    InsertRecord(queue, temporary, runtime);
    if (temporary.owns_callback != 0u && temporary.callback != 0u)
        runtime.DeleteCallback(temporary.callback, 1u);
    if (temporary.name_data == 0u) return 0u;
    return runtime.ReleaseNameResult(temporary);
}

TimedQueueRecord& TimedQueueRecordCopier::CopyAssign(
    TimedQueueRecord& destination, TimedQueueRecord& source,
    ITimedQueueRecordCopyRuntime& runtime) {
    destination.name_flag =
        (destination.name_flag & 0xffffff00u) | (source.name_flag & 0xffu);
    destination.name_data = 0u;
    destination.name_length = 0u;
    destination.name_capacity = 0u;
    const auto length = source.name_length;

    if (&destination == &source) {
        runtime.EraseName(destination, length, 0xffffffffu);
        runtime.EraseName(destination, 0u, 0u);
    } else {
        const auto name_data = [&]() {
            return source.name_data != 0u
                       ? source.name_data : runtime.EmptyString();
        };
        bool share = false;
        if (length != 0u)
            share = runtime.ReadByte(name_data() - 1u) < 0xfeu;
        if (share) {
            runtime.SetNameErrorFlag(destination, 1u);
            const auto data = name_data();
            destination.name_data = data;
            destination.name_length = source.name_length;
            destination.name_capacity = source.name_capacity;
            runtime.WriteByte(data - 1u,
                              static_cast<std::uint8_t>(
                                  runtime.ReadByte(data - 1u) + 1u));
        } else if (runtime.AllocateName(destination, length, 1u)) {
            runtime.CopyBytes(destination.name_data, name_data(), length);
            runtime.SetNameSize(destination, length);
        }
    }

    destination.delay = source.delay;
    destination.owns_callback =
        (destination.owns_callback & 0xffffff00u) |
        (source.owns_callback & 0xffu);
    const auto callback = source.callback;
    source.owns_callback &= 0xffffff00u;
    destination.callback = callback;
    return destination;
}

}  // namespace pal4
