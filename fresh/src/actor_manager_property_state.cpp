#include "pal4/actor_manager_property_state.h"

namespace pal4 {

namespace {

void Assign(std::uint32_t actor, ActorPropertyHandle& incoming,
            IActorManagerPropertyStateRuntime& runtime) {
    const auto current = runtime.ReadWord(actor + 800u);
    if (current == incoming.property) {
        if (incoming.owned) runtime.WriteByte(actor + 796u, 1u);
    } else {
        if (runtime.ReadByte(actor + 796u) != 0u && current != 0u)
            (void)runtime.ReleaseProperty(current);
        runtime.WriteByte(actor + 796u, incoming.owned ? 1u : 0u);
    }
    incoming.owned = false;
    runtime.WriteWord(actor + 800u, incoming.property);
}

}  // namespace

std::uint8_t ActorManagerPropertyState::Set(
    std::uint32_t actor, ActorPropertyHandle incoming,
    IActorManagerPropertyStateRuntime& runtime) {
    if (incoming.property != 0u) {
        const ActorPropertyHandle previous{
            runtime.ReadByte(actor + 796u) != 0u,
            runtime.ReadWord(actor + 800u)};
        runtime.WriteByte(actor + 796u, 0u);
        runtime.NotifyPropertyChange(actor, previous, incoming);
        Assign(actor, incoming, runtime);
        const auto property = runtime.ReadWord(actor + 800u);
        runtime.RefreshPropertyCount(property, actor);
        runtime.BindProperty(property, actor);
    }
    if (incoming.owned && incoming.property != 0u)
        return runtime.ReleaseProperty(incoming.property);
    return static_cast<std::uint8_t>(incoming.owned);
}

ActorPropertyHandle ActorManagerPropertyState::MoveEmbedded(
    std::uint32_t property, IActorManagerPropertyStateRuntime& runtime) {
    const ActorPropertyHandle result{
        runtime.ReadByte(property + 4u) != 0u,
        runtime.ReadWord(property + 8u)};
    runtime.WriteByte(property + 4u, 0u);
    return result;
}

void ActorManagerPropertyState::HandleEvent(
    std::uint32_t actor, std::uint32_t event_id,
    IActorManagerPropertyStateRuntime& runtime) {
    const auto current = runtime.ReadWord(actor + 800u);
    if (runtime.PropertyId(current) != event_id) {
        runtime.DispatchProperty(current, actor, event_id);
        return;
    }

    const bool previous_owned = runtime.ReadByte(actor + 796u) != 0u;
    runtime.WriteByte(actor + 796u, 0u);
    auto embedded = MoveEmbedded(current, runtime);
    Assign(actor, embedded, runtime);
    runtime.FinishProperty(current, actor);
    HandleEvent(actor, event_id, runtime);
    if (previous_owned) (void)runtime.ReleaseProperty(current);
}

bool ActorManagerPropertyState::IsIdle(
    std::uint32_t actor, IActorManagerPropertyStateRuntime& runtime) {
    return runtime.ReadWord(actor + 828u) == 0u;
}

bool ActorManagerPropertyState::IsReady(
    std::uint32_t actor, IActorManagerPropertyStateRuntime& runtime) {
    const auto state = runtime.ReadWord(actor + 828u);
    return (state == 1u || state == 2u || state == 5u) &&
           !runtime.HasBlockingObjectState(actor);
}

bool ActorManagerPropertyState::IsUnavailable(
    std::uint32_t actor, IActorManagerPropertyStateRuntime& runtime) {
    const auto state = runtime.ReadWord(actor + 828u);
    return state == 3u || state == 4u || state == 6u || state == 7u;
}

std::uint32_t ActorManagerPropertyState::ForwardSlot236(
    std::uint32_t actor, IActorManagerPropertyStateRuntime& runtime) {
    return runtime.InvokePropertySlot(runtime.ReadWord(actor + 800u),
                                      actor, 236u, 0u);
}

namespace {

std::uint32_t ProcessAction(
    std::uint32_t actor, std::uint32_t argument, std::uint32_t slot,
    std::uint32_t variant, IActorManagerPropertyStateRuntime& runtime) {
    const auto result = runtime.InvokePropertySlot(
        runtime.ReadWord(actor + 800u), actor, slot, argument);
    if (result == 0u)
        runtime.CleanupConditionalList(actor, 39u + variant, 0u);

    auto generated = runtime.MakePropertyHandle(variant, result);
    if (generated.property != 0u) {
        (void)ActorManagerPropertyState::Set(actor, generated, runtime);
        generated.owned = false;
    } else {
        ActorManagerPropertyState::HandleEvent(actor, variant, runtime);
    }
    if (variant == 2u) {
        const auto mode = runtime.ActorMode(actor);
        if (runtime.ReadWord(actor + 808u) != 0u)
            runtime.SetClumpAlpha(mode == 4u ? 0.05f : 1.0f,
                                  runtime.CombatActorData());
    }
    if (generated.owned && generated.property != 0u)
        (void)runtime.ReleaseProperty(generated.property);
    return result;
}

}  // namespace

std::uint32_t ActorManagerPropertyState::ProcessPropertyAction(
    std::uint32_t actor, std::uint32_t argument,
    IActorManagerPropertyStateRuntime& runtime) {
    return ProcessAction(actor, argument, 240u, 0u, runtime);
}

std::uint32_t ActorManagerPropertyState::ForwardSlot244(
    std::uint32_t actor, IActorManagerPropertyStateRuntime& runtime) {
    return runtime.InvokePropertySlot(runtime.ReadWord(actor + 800u),
                                      actor, 244u, 0u);
}

std::uint32_t ActorManagerPropertyState::ProcessPropertyAction1(
    std::uint32_t actor, std::uint32_t argument,
    IActorManagerPropertyStateRuntime& runtime) {
    return ProcessAction(actor, argument, 248u, 1u, runtime);
}

std::uint32_t ActorManagerPropertyState::ForwardSlot252(
    std::uint32_t actor, IActorManagerPropertyStateRuntime& runtime) {
    return runtime.InvokePropertySlot(runtime.ReadWord(actor + 800u),
                                      actor, 252u, 0u);
}

std::uint32_t ActorManagerPropertyState::ProcessPropertyAction2(
    std::uint32_t actor, std::uint32_t argument,
    IActorManagerPropertyStateRuntime& runtime) {
    return ProcessAction(actor, argument, 256u, 2u, runtime);
}

std::uint32_t ActorManagerPropertyState::ForwardSlot260(
    std::uint32_t actor, IActorManagerPropertyStateRuntime& runtime) {
    return runtime.InvokePropertySlot(runtime.ReadWord(actor + 800u),
                                      actor, 260u, 0u);
}

std::uint32_t ActorManagerPropertyState::ProcessPropertyAction3(
    std::uint32_t actor, std::uint32_t argument,
    IActorManagerPropertyStateRuntime& runtime) {
    return ProcessAction(actor, argument, 264u, 3u, runtime);
}

std::uint32_t ActorManagerPropertyState::ForwardSlot268(
    std::uint32_t actor, IActorManagerPropertyStateRuntime& runtime) {
    return runtime.InvokePropertySlot(runtime.ReadWord(actor + 800u),
                                      actor, 268u, 0u);
}

std::uint32_t ActorManagerPropertyState::ProcessPropertyAction4(
    std::uint32_t actor, std::uint32_t argument,
    IActorManagerPropertyStateRuntime& runtime) {
    if (argument == 3u && runtime.CheckReviveStatus(actor)) {
        runtime.ClearReviveStatus(actor);
        const auto value = runtime.InvokeActorSlot48(actor, 0u);
        runtime.InvokeActorSlot68(actor, 7u, value);
        runtime.DispatchRevive(actor);
        runtime.InvokeActorSlot272(actor, 7432u);
        return runtime.InvokeActorSlot168(actor);
    }
    return ProcessAction(actor, argument, 272u, 4u, runtime);
}

std::uint32_t ActorManagerPropertyState::ForwardSlot276(
    std::uint32_t actor, IActorManagerPropertyStateRuntime& runtime) {
    return runtime.InvokePropertySlot(runtime.ReadWord(actor + 800u),
                                      actor, 276u, 0u);
}

std::uint32_t ActorManagerPropertyState::ProcessPropertyAction5(
    std::uint32_t actor, std::uint32_t argument,
    IActorManagerPropertyStateRuntime& runtime) {
    return ProcessAction(actor, argument, 280u, 5u, runtime);
}

std::uint32_t ActorManagerPropertyState::ForwardSlot284(
    std::uint32_t actor, IActorManagerPropertyStateRuntime& runtime) {
    return runtime.InvokePropertySlot(runtime.ReadWord(actor + 800u),
                                      actor, 284u, 0u);
}

std::uint32_t ActorManagerPropertyState::ProcessPropertyAction6(
    std::uint32_t actor, std::uint32_t argument,
    IActorManagerPropertyStateRuntime& runtime) {
    return ProcessAction(actor, argument, 288u, 6u, runtime);
}

std::uint32_t ActorManagerPropertyState::ForwardSlot292(
    std::uint32_t actor, IActorManagerPropertyStateRuntime& runtime) {
    return runtime.InvokePropertySlot(runtime.ReadWord(actor + 800u),
                                      actor, 292u, 0u);
}

std::uint32_t ActorManagerPropertyState::ProcessPropertyAction7(
    std::uint32_t actor, std::uint32_t argument,
    IActorManagerPropertyStateRuntime& runtime) {
    return ProcessAction(actor, argument, 296u, 7u, runtime);
}

std::uint32_t ActorManagerPropertyState::ForwardSlot300(
    std::uint32_t actor, IActorManagerPropertyStateRuntime& runtime) {
    return runtime.InvokePropertySlot(runtime.ReadWord(actor + 800u),
                                      actor, 300u, 0u);
}

std::uint32_t ActorManagerPropertyState::ProcessPropertyAction8(
    std::uint32_t actor, std::uint32_t argument,
    IActorManagerPropertyStateRuntime& runtime) {
    return ProcessAction(actor, argument, 304u, 8u, runtime);
}

}  // namespace pal4
