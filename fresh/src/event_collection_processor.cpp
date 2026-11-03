#include "pal4/event_collection_processor.h"

#include <new>

namespace pal4 {

std::uint32_t EventCollectionRuntimeStub::Allocate(std::uint32_t) { return 0u; }
std::uint32_t EventCollectionRuntimeStub::ReadWord(std::uint32_t) { return 0u; }
void EventCollectionRuntimeStub::WriteWord(std::uint32_t, std::uint32_t) {}
void EventCollectionRuntimeStub::Free(std::uint32_t) {}
bool EventCollectionRuntimeStub::AtEnd(std::uint32_t, std::uint32_t) {
    return true;
}
void EventCollectionRuntimeStub::Advance(std::uint32_t, std::uint32_t) {}
void EventCollectionRuntimeStub::Visit(std::uint32_t, EventVisitorState&,
                                        std::uint32_t) {}
void EventCollectionRuntimeStub::Append(EventOutputState&, std::uint32_t) {}
void EventCollectionRuntimeStub::IncrementTempOff() {}

std::uint32_t EventCollectionProcessor::Process(
    EventOutputState& output, const std::uint32_t collection,
    IEventCollectionRuntime& runtime) {
    std::uint32_t first = 0u;
    (void)CollectionIteratorLifecycle::CreateFromFirst(
        collection, first, runtime);
    if (first == 0u) throw std::bad_alloc{};
    try {
        while (!runtime.AtEnd(first, 24u)) {
            const auto element = CollectionIteratorLifecycle::GetValue(
                first, runtime);
            EventVisitorState visitor{};
            runtime.Visit(element, visitor, 12u);
            runtime.Append(output, visitor.output);
            runtime.Advance(first, 36u);
        }

        std::uint32_t third = 0u;
        (void)CollectionIteratorLifecycle::CreateFromThird(
            collection, third, runtime);
        if (third == 0u) throw std::bad_alloc{};
        try {
            while (!runtime.AtEnd(third, 24u)) {
                const auto parent = CollectionIteratorLifecycle::GetValue(
                    third, runtime);
                EventOutputState children{};
                children.tag = third & 0xffu;
                try {
                    (void)Process(children, runtime.ReadWord(parent + 4u),
                                  runtime);
                    for (auto cursor = children.begin; cursor != children.end;
                         cursor += 4u) {
                        const auto wrapper = runtime.Allocate(12u);
                        if (wrapper != 0u) {
                            runtime.WriteWord(wrapper, 0x849898u);
                            runtime.IncrementTempOff();
                            runtime.WriteWord(wrapper + 8u,
                                              runtime.ReadWord(cursor));
                            runtime.WriteWord(wrapper, 0x849888u);
                            runtime.WriteWord(wrapper + 4u, parent);
                        }
                        try {
                            runtime.Append(output, wrapper);
                        } catch (...) {
                            if (wrapper != 0u) runtime.Free(wrapper);
                            throw;
                        }
                    }
                } catch (...) {
                    runtime.Free(children.begin);
                    throw;
                }
                runtime.Free(children.begin);
                runtime.Advance(third, 36u);
            }
        } catch (...) {
            (void)CollectionIteratorLifecycle::CleanupFirst(
                third, 1u, runtime);
            throw;
        }
        (void)CollectionIteratorLifecycle::CleanupFirst(third, 1u, runtime);
    } catch (...) {
        (void)CollectionIteratorLifecycle::CleanupSecond(first, 1u, runtime);
        throw;
    }
    return CollectionIteratorLifecycle::CleanupSecond(first, 1u, runtime);
}

std::uint8_t EventCollectionProcessor::ProcessAndBind(
    const std::uint32_t target, const std::uint32_t collection,
    IEventCollectionBindRuntime& runtime) {
    EventOutputState output{};
    output.tag = collection & 0xffu;
    try {
        (void)Process(output, collection, runtime);
        for (auto cursor = output.begin; cursor != output.end;
             cursor += 4u)
            runtime.BindEffect(target, runtime.ReadWord(cursor));
    } catch (...) {
        runtime.Free(output.begin);
        throw;
    }
    runtime.Free(output.begin);
    return 1u;
}

}  // namespace pal4
