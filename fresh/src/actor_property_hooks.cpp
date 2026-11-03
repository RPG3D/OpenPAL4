#include "pal4/actor_property_hooks.h"

namespace pal4 {
namespace {

std::int32_t DispatchFacet(std::uint32_t actor,
                           IActorPropertyHookRuntime& runtime) {
    const auto record = runtime.TemporaryRecord();
    (void)ActorFacetEventRecord::ConstructFacet(record, actor, runtime);
    const auto key = runtime.ReadWord(0x8b59c0u);
    return runtime.DispatchEvent(runtime.EventRouter(), key, record, 1u);
}

std::int32_t SubtractWrap(std::int32_t left, std::int32_t right) {
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(left) -
        static_cast<std::uint32_t>(right));
}

}  // namespace

std::uint32_t ActorPropertyHooks::SetPendingPointer(
    std::uint32_t object, std::uint32_t value,
    IActorPropertyHookRuntime& runtime) {
    runtime.WriteWord(object + 12u, value);
    return value;
}

std::int32_t ActorPropertyHooks::ForwardWithPendingAdjustment(
    std::uint32_t object, std::uint32_t actor,
    std::uint32_t index, std::int32_t value,
    IActorPropertyHookRuntime& runtime) {
    auto forwarded = value;
    if (index == 7u && runtime.ReadWord(object + 12u) != 0u) {
        const auto current = runtime.ActorIntVirtual(actor, 52u, 7u);
        const auto delta = SubtractWrap(value, current);
        if (delta < 0) {
            forwarded = runtime.ActorIntVirtual(actor, 52u, 7u);
            runtime.PendingAdjustVirtual(runtime.ReadWord(object + 12u),
                                         84u, 7u, delta);
        }
        runtime.WriteWord(object + 12u, 0u);
    }
    const auto owner = runtime.ReadWord(object + 8u);
    return runtime.OwnerSetVirtual(owner, 168u, actor, index, forwarded);
}

std::uint32_t ActorPropertyHooks::ConstructPendingPointer(
    std::uint32_t object, IActorPropertyHookRuntime& runtime) {
    runtime.WriteByte(object + 4u, 0u);
    runtime.WriteWord(object + 8u, 0u);
    runtime.WriteWord(object, 0x8463f0u);
    runtime.WriteWord(object + 12u, 0u);
    return object;
}

std::int32_t ActorPropertyHooks::DispatchThenSetPending(
    std::uint32_t object, std::uint32_t actor,
    std::uint32_t value, IActorPropertyHookRuntime& runtime) {
    const auto result = DispatchFacet(actor, runtime);
    runtime.WriteWord(object + 12u, value);
    return result;
}

std::uint32_t ActorPropertyHooks::ConstructPendingGate(
    std::uint32_t object, IActorPropertyHookRuntime& runtime) {
    runtime.WriteByte(object + 4u, 0u);
    runtime.WriteWord(object + 8u, 0u);
    runtime.WriteWord(object, 0x846550u);
    return object;
}

std::int32_t ActorPropertyHooks::ForwardUnlessPending(
    std::uint32_t object, std::uint32_t actor,
    std::uint32_t index, std::int32_t value,
    IActorPropertyHookRuntime& runtime) {
    if (index == 7u && runtime.ReadWord(object + 12u) != 0u) {
        runtime.WriteWord(object + 12u, 0u);
        return 0;
    }
    const auto owner = runtime.ReadWord(object + 8u);
    return runtime.OwnerSetVirtual(owner, 168u, actor, index, value);
}

std::uint32_t ActorPropertyHooks::ConstructReactiveGate(
    std::uint32_t object, IActorPropertyHookRuntime& runtime) {
    runtime.WriteByte(object + 4u, 0u);
    runtime.WriteWord(object + 8u, 0u);
    runtime.WriteByte(object + 12u, 0u);
    runtime.WriteWord(object, 0x8466b0u);
    return object;
}

void ActorPropertyHooks::NotifyReactiveGate(
    std::uint32_t object, std::uint32_t event_id,
    IActorPropertyHookRuntime& runtime) {
    if (event_id == 9116u) runtime.WriteByte(object + 12u, 1u);
}

std::int32_t ActorPropertyHooks::ForwardWithReactiveAdjustment(
    std::uint32_t object, std::uint32_t actor,
    std::uint32_t index, std::int32_t value,
    IActorPropertyHookRuntime& runtime) {
    auto forwarded = value;
    if (index == 7u && runtime.ReadByte(object + 12u) != 0u) {
        const auto current = runtime.ActorIntVirtual(actor, 52u, 7u);
        const auto delta = SubtractWrap(value, current);
        if (delta < 0) {
            const auto reread = runtime.ActorIntVirtual(actor, 52u, 7u);
            forwarded = SubtractWrap(reread, delta);
        }
    }
    const auto owner = runtime.ReadWord(object + 8u);
    return runtime.OwnerSetVirtual(owner, 168u, actor, index, forwarded);
}

std::int32_t ActorPropertyHooks::DispatchThenFinish(
    std::uint32_t object, std::uint32_t actor,
    IActorPropertyHookRuntime& runtime) {
    (void)DispatchFacet(actor, runtime);
    const auto owner = runtime.ReadWord(object + 8u);
    return runtime.OwnerFinishVirtual(owner, 240u, actor, 0u);
}

}  // namespace pal4
