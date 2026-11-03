#include "pal4/jigsaw_event_infrastructure.h"

#include <new>

namespace pal4 {

namespace {
std::uint32_t ConstructEventCallback(
    const std::uint32_t object, const std::uint32_t first,
    const std::uint32_t second, const std::uint32_t third,
    const std::uint32_t vtable, IJigsawEventCallbackRuntime& runtime) {
    runtime.WriteWord(object + 8u, first);
    runtime.WriteWord(object + 0xcu, second);
    runtime.WriteWord(object + 0x10u, third);
    runtime.WriteWord(object, vtable);
    return object;
}
}  // namespace

std::uint32_t JigsawRecordVector::Clear(
    const std::uint32_t vector, IJigsawRecordVectorRuntime& runtime) {
    const auto end = runtime.ReadWord(vector + 8u);
    const auto begin = runtime.ReadWord(vector + 4u);
    for (auto record = begin; record != end; record += 0xc8u)
        DestroyRecord(record, runtime);
    (void)runtime.Free(begin);
    runtime.WriteWord(vector + 4u, 0u);
    runtime.WriteWord(vector + 8u, 0u);
    runtime.WriteWord(vector + 0xcu, 0u);
    return 0u;
}

std::uint32_t JigsawRecordVector::FreeBuffer(
    const std::uint32_t buffer, const std::uint32_t,
    IJigsawRecordVectorRuntime& runtime) {
    return runtime.Free(buffer);
}

void JigsawRecordVector::DestroyRecord(
    const std::uint32_t record, IJigsawRecordVectorRuntime& runtime) {
    runtime.DestroyString(record + 0x10u);
}

std::uint32_t EventQueue::Add(
    const std::uint32_t queue, const std::uint32_t source,
    IEventQueueRuntime& runtime) {
    const auto head = runtime.ReadWord(queue + 4u);
    const auto successor = runtime.ReadWord(head + 4u);
    const auto node = runtime.Allocate(0x24u);
    if (node == 0u) throw std::bad_alloc();
    runtime.WriteWord(node, head);
    runtime.WriteWord(node + 4u, successor != 0u ? successor : node);
    runtime.WriteWord(head + 4u, node);
    runtime.WriteWord(runtime.ReadWord(node + 4u), node);
    runtime.CopyEventData(node + 8u, source);
    const auto count = runtime.ReadWord(queue + 8u) + 1u;
    runtime.WriteWord(queue + 8u, count);
    return count;
}

std::uint32_t JigsawEventCallback::Construct(
    const std::uint32_t object, const std::uint32_t first,
    const std::uint32_t second, const std::uint32_t third,
    IJigsawEventCallbackRuntime& runtime) {
    return ConstructEventCallback(object, first, second, third,
                                  kVtable, runtime);
}

std::uint32_t PuzzleEventCallback::Construct(
    const std::uint32_t object, const std::uint32_t first,
    const std::uint32_t second, const std::uint32_t third,
    IJigsawEventCallbackRuntime& runtime) {
    return ConstructEventCallback(object, first, second, third,
                                  kVtable, runtime);
}

std::uint32_t JigsawEventCallback::Invoke(
    const std::uint32_t object, IJigsawEventCallbackRuntime& runtime) {
    return runtime.InvokeVirtual(object, 4u);
}

}  // namespace pal4
