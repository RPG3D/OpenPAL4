#include "pal4/actor_facet_event_record.h"

namespace pal4 {

std::uint32_t ActorFacetEventRecord::ActorFromFacetValue(std::uint32_t facet) {
    return facet==0u?0u:facet-kActorFacetOffset;
}
std::uint32_t ActorFacetEventRecord::ConstructRaw(
    const std::uint32_t record, const std::uint32_t value,
    IActorFacetEventRecordRuntime& runtime) {
    runtime.WriteWord(record, value);
    return record;
}

std::uint32_t ActorFacetEventRecord::ConstructFacet(
    const std::uint32_t record, const std::uint32_t actor,
    IActorFacetEventRecordRuntime& runtime) {
    return ConstructRaw(record,
                        actor == 0u ? 0u : actor + kActorFacetOffset,
                        runtime);
}

std::uint32_t ActorFacetEventRecord::ActorFromFacet(
    const std::uint32_t record,
    IActorFacetEventRecordRuntime& runtime) {
    const auto facet = runtime.ReadWord(record);
    return ActorFromFacetValue(facet);
}

std::uint32_t ActorFacetEventRecord::ConstructFourWords(
    const std::uint32_t record, const std::uint32_t actor,
    const std::uint32_t second, const std::uint32_t third,
    const std::uint32_t fourth, IActorFacetEventRecordRuntime& runtime) {
    (void)ConstructFacet(record, actor, runtime);
    runtime.WriteWord(record + 12u, fourth);
    runtime.WriteWord(record + 4u, second);
    runtime.WriteWord(record + 8u, third);
    return record;
}

std::uint32_t ActorFacetEventRecord::ConstructFixedMode(
    const std::uint32_t record, const std::uint32_t actor,
    IActorFacetEventRecordRuntime& runtime) {
    (void)ConstructFacet(record, actor, runtime);
    runtime.WriteWord(record + 12u, 6u);
    runtime.WriteWord(record + 4u, 0u);
    runtime.WriteWord(record + 8u, 0u);
    return record;
}

std::uint32_t ActorFacetEventRecord::ConstructPair(
    const std::uint32_t record, const std::uint32_t actor,
    const std::uint32_t second, const std::uint32_t third,
    IActorFacetEventRecordRuntime& runtime) {
    (void)ConstructFacet(record, actor, runtime);
    runtime.WriteWord(record + 4u, second);
    runtime.WriteWord(record + 8u, third);
    return record;
}

std::uint32_t ActorFacetEventRecord::ConstructOneExtra(
    const std::uint32_t record, const std::uint32_t actor,
    const std::uint32_t second, IActorFacetEventRecordRuntime& runtime) {
    (void)ConstructFacet(record, actor, runtime);
    runtime.WriteWord(record + 4u, second);
    return record;
}

std::uint32_t ActorFacetEventRecord::ConstructWithEffectHandle(
    const std::uint32_t record, const std::uint32_t actor,
    std::uint32_t handle_first, std::uint32_t handle_second,
    IActorFacetEventRecordRuntime& runtime) {
    (void)ConstructFacet(record, actor, runtime);
    runtime.WriteWord(record + 4u, handle_first);
    runtime.WriteWord(record + 8u, handle_second);
    runtime.ResetTemporaryHandle(handle_first, handle_second);
    return record;
}

}  // namespace pal4
