#include "pal4/actor_manager_player_event_dispatch.h"

namespace pal4 {

std::int32_t ActorManagerPlayerEventDispatch::AddMoney(
    const std::uint32_t amount,
    IActorManagerPlayerEventRuntime& runtime) {
    const auto first_router = runtime.EventRouter();
    (void)runtime.Dispatch(first_router, "add_money", amount, 0u, 1u, 1u);
    const auto second_router = runtime.EventRouter();
    return runtime.Dispatch(second_router, "add_picked_money",
                            amount, 0u, 1u, 1u);
}

std::int32_t ActorManagerPlayerEventDispatch::AddProperty(
    const std::uint32_t actor, const std::uint32_t property,
    const std::uint32_t amount,
    IActorManagerPlayerEventRuntime& runtime) {
    if (!property) return runtime.ActorSlot344(actor);
    const auto first_router = runtime.EventRouter();
    (void)runtime.Dispatch(first_router, "add_property",
                           property, amount, 2u, 1u);
    const auto second_router = runtime.EventRouter();
    return runtime.Dispatch(second_router, "add_picked_property",
                            property, amount, 2u, 1u);
}

std::int32_t ActorManagerPlayerEventDispatch::SendAddProperty(
    const std::uint32_t property, const std::uint32_t amount,
    IActorManagerPlayerEventRuntime& runtime) {
    const auto router = runtime.EventRouter();
    return runtime.Dispatch(router, "add_property", property, amount, 2u, 1u);
}

std::int32_t ActorManagerPlayerEventDispatch::SendAddMoney(
    const std::uint32_t amount,
    IActorManagerPlayerEventRuntime& runtime) {
    const auto router = runtime.EventRouter();
    return runtime.Dispatch(router, "add_money", amount, 0u, 1u, 1u);
}

std::int32_t ActorManagerPlayerEventDispatch::PickUpFail(
    const std::uint32_t receiver,
    IActorManagerPlayerEventRuntime& runtime) {
    const auto router = runtime.EventRouter();
    return runtime.DispatchByte(
        router, "pick_up_fail", static_cast<std::uint8_t>(receiver >> 24u),
        1u);
}

}  // namespace pal4
