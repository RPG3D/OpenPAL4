#include "pal4/actor_manager_action_events.h"

namespace pal4 {

std::uint32_t ActorManagerActionEvents::TransferMetric8(
    const std::uint32_t actor, const std::uint32_t target,
    const float factor, IActorManagerActionEventRuntime& runtime) {
    if (runtime.ReadWord(actor + 804u) ==
        runtime.ReadWord(target + 804u)) return 1u;
    const auto first = runtime.ReadMetric(actor, target, 432u);
    const auto second = runtime.ReadMetric(actor, target, 428u);
    const auto first_value = static_cast<std::uint32_t>(
        runtime.FloatToInteger(static_cast<float>(first) * factor));
    (void)runtime.AddMetric8(actor, first_value);
    const auto second_value = static_cast<std::uint32_t>(
        runtime.FloatToInteger(static_cast<float>(second) * factor));
    return runtime.AddMetric8(target, second_value);
}

std::uint32_t ActorManagerActionEvents::StartAction(
    const std::uint32_t actor, IActorManagerActionLifecycleRuntime& runtime) {
    runtime.HandleEvent14(actor, 2u, 1u);
    (void)runtime.Dispatch("ae_start_action", actor, 1u);
    return runtime.InvokePropertySlot(runtime.ReadWord(actor + 800u),
                                      28u, actor, 0u);
}

std::uint32_t ActorManagerActionEvents::EndAction(
    const std::uint32_t actor, IActorManagerActionLifecycleRuntime& runtime) {
    if (runtime.ReadWord(actor + 828u) == 2u) {
        runtime.HandleEvent14(actor, 1u, 1u);
        runtime.InvokeActorSlot396(actor, 1u);
    }
    (void)runtime.Dispatch("ae_end_action", actor, 1u);
    return runtime.InvokePropertySlot(runtime.ReadWord(actor + 800u),
                                      32u, actor, 0u);
}

std::uint8_t ActorManagerActionEvents::ResetForTarget(
    const std::uint32_t actor, const std::uint32_t target,
    IActorManagerActionEventRuntime& runtime) {
    if (runtime.ReadWord(actor + 804u) ==
        runtime.ReadWord(target + 804u)) return 1u;
    const auto state = runtime.InvokeActorSlot144(actor);
    const auto result = state == 4u ?
        runtime.InvokeActorSlot148(actor, 0u) : state;
    return static_cast<std::uint8_t>(result);
}

namespace {
std::uint32_t Forward(const std::uint32_t actor,
                      const std::uint32_t target,
                      const std::uint32_t slot,
                      IActorManagerActionEventRuntime& runtime) {
    (void)ActorManagerActionEvents::ResetForTarget(actor, target, runtime);
    return runtime.InvokePropertySlot(runtime.ReadWord(actor + 800u),
                                      slot, actor, target);
}
}  // namespace

std::uint32_t ActorManagerActionEvents::ForwardSlot36(
    const std::uint32_t actor, const std::uint32_t target,
    IActorManagerActionEventRuntime& runtime) {
    return Forward(actor, target, 36u, runtime);
}

std::uint32_t ActorManagerActionEvents::ForwardSlot40(
    const std::uint32_t actor, const std::uint32_t target,
    IActorManagerActionEventRuntime& runtime) {
    return Forward(actor, target, 40u, runtime);
}

std::uint32_t ActorManagerActionEvents::ForwardSlot44(
    const std::uint32_t actor, const std::uint32_t target,
    IActorManagerActionEventRuntime& runtime) {
    return Forward(actor, target, 44u, runtime);
}

std::uint32_t ActorManagerActionEvents::ForwardSlot48(
    const std::uint32_t actor, const std::uint32_t target,
    IActorManagerActionEventRuntime& runtime) {
    return Forward(actor, target, 48u, runtime);
}

}  // namespace pal4
