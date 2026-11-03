#include "pal4/actor_manager_data_events.h"

namespace pal4 {

bool ActorManagerDataEvents::Process(
    const std::uint32_t actor, IActorReadyCandidateRuntime& runtime) {
    if (!runtime.IsActorReady(actor)) return false;
    const auto iterator = runtime.CreateIterator(actor);
    bool result{};
    try {
        while (!runtime.IteratorAtEnd(iterator)) {
            if (!runtime.ObjectCheckState3(runtime.IteratorCurrent(iterator))) {
                const auto property = runtime.ReadWord(actor + 800u);
                result = static_cast<std::uint8_t>(runtime.InvokePropertySlot(
                    property, 4u, actor, 0u)) != 0u;
                break;
            }
            runtime.IteratorNext(iterator);
        }
    } catch (...) {
        if (iterator) runtime.DestroyIterator(iterator, 1u);
        throw;
    }
    if (iterator) runtime.DestroyIterator(iterator, 1u);
    return result;
}

std::uint32_t ActorManagerDataEvents::CallPropertySlot8(
    const std::uint32_t actor, const std::uint32_t argument,
    IActorPropertySlotRuntime& runtime) {
    return runtime.InvokePropertySlot(runtime.ReadWord(actor + 800u),
                                      8u, actor, argument);
}

std::uint32_t ActorManagerDataEvents::CallPropertySlot12(
    const std::uint32_t actor, const std::uint32_t argument,
    IActorPropertySlotRuntime& runtime) {
    return runtime.InvokePropertySlot(runtime.ReadWord(actor + 800u),
                                      12u, actor, argument);
}

bool ActorManagerDataEvents::CheckMetric8(
    const std::uint32_t actor, const std::uint32_t candidate,
    IActorMetricRuntime& runtime) {
    return static_cast<std::int32_t>(runtime.ReadWord(candidate + 116u)) <=
           runtime.ActorMetric(actor, 8u);
}

bool ActorManagerDataEvents::CheckMetric9(
    const std::uint32_t actor, const std::uint32_t candidate,
    IActorMetricRuntime& runtime) {
    return static_cast<std::int32_t>(runtime.ReadWord(candidate + 76u)) <=
           runtime.ActorMetric(actor, 9u);
}

bool ActorManagerDataEvents::SharesOwner(
    const std::uint32_t actor, const std::uint32_t candidate,
    IActorManagerDataEventRuntime& runtime) {
    return runtime.ReadWord(actor + 804u) ==
           runtime.ReadWord(candidate + 804u);
}

}  // namespace pal4
