#include "pal4/combat_actor_property_callbacks.h"

#include <cstdint>

namespace pal4 {
namespace {

std::int32_t WrapSubtract(std::int32_t left, std::int32_t right) {
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(left) -
        static_cast<std::uint32_t>(right));
}

std::int32_t LegacyFtol(double value) {
    return static_cast<std::int32_t>(static_cast<std::int64_t>(value));
}

std::int32_t Forward(std::uint32_t callback, std::uint32_t actor,
                     std::uint32_t index, std::int32_t value,
                     ICombatActorPropertyRuntime& runtime) {
    return runtime.OwnerVirtual168(
        runtime.ReadWord(callback + 8u), actor, index, value);
}

}  // namespace

std::int32_t CombatActorPropertyCallbacks::AdjustSlotNineDecrease(
    std::uint32_t callback, std::uint32_t actor,
    std::uint32_t index, std::int32_t value,
    ICombatActorPropertyRuntime& runtime) {
    auto adjusted = value;
    if (index == 9u) {
        const auto current = runtime.ActorVirtual52(actor, 9u);
        if (current > value) {
            const auto reread = runtime.ActorVirtual52(actor, 9u);
            const auto delta = WrapSubtract(value, reread);
            const auto scaled = LegacyFtol(
                static_cast<double>(delta) *
                static_cast<double>(-1.2999999523162842f));
            adjusted = WrapSubtract(reread, scaled);
        }
    }
    return Forward(callback, actor, index, adjusted, runtime);
}

std::int32_t CombatActorPropertyCallbacks::ApplySlotNinePercent(
    std::uint32_t actor, ICombatActorPropertyRuntime& runtime) {
    const auto slot_two = runtime.ActorVirtual48(actor, 2u);
    const auto threshold = static_cast<float>(
        static_cast<double>(slot_two) *
        static_cast<double>(0.05000000074505806f));
    if (static_cast<double>(runtime.ActorVirtual52(actor, 9u)) <
        static_cast<double>(threshold))
        return runtime.ActorVirtual68(actor, 9u, 1);
    return runtime.ActorVirtual84(actor, 9u,
                                  LegacyFtol(-static_cast<double>(threshold)));
}

std::uint8_t CombatActorPropertyCallbacks::NotifyDifferentOwner(
    std::uint32_t actor, std::uint32_t other,
    ICombatActorPropertyRuntime& runtime) {
    const auto shared = runtime.SharesOwner(actor, other);
    if (shared != 0u) return shared;
    return static_cast<std::uint8_t>(
        runtime.ActorVirtual268(actor, 7312u, actor));
}

std::int32_t CombatActorPropertyCallbacks::ApplySlotSevenPercent(
    std::uint32_t actor, ICombatActorPropertyRuntime& runtime) {
    const auto slot_zero = runtime.ActorVirtual48(actor, 0u);
    const auto amount = LegacyFtol(
        static_cast<double>(slot_zero) *
        static_cast<double>(0.05000000074505806f));
    if (runtime.ActorVirtual52(actor, 7u) < amount)
        return runtime.ActorVirtual68(actor, 7u, 1);
    return runtime.ActorVirtual84(actor, 7u, WrapSubtract(0, amount));
}

std::int32_t CombatActorPropertyCallbacks::ReflectSlotSevenExcess(
    std::uint32_t callback, std::uint32_t actor,
    std::uint32_t index, std::int32_t value,
    ICombatActorPropertyRuntime& runtime) {
    auto adjusted = value;
    if (index == 7u && value > runtime.ActorVirtual52(actor, 7u)) {
        const auto current = runtime.ActorVirtual52(actor, 7u);
        adjusted = WrapSubtract(
            static_cast<std::int32_t>(
                static_cast<std::uint32_t>(current) * 2u), value);
    }
    return Forward(callback, actor, index, adjusted, runtime);
}

std::int32_t CombatActorPropertyCallbacks::ScaleSlotSevenNineExcess(
    std::uint32_t callback, std::uint32_t actor,
    std::uint32_t index, std::int32_t value,
    ICombatActorPropertyRuntime& runtime) {
    auto adjusted = value;
    if (index == 7u || index == 9u) {
        const auto current = runtime.ActorVirtual52(actor, index);
        if (value > current) {
            const auto reread = runtime.ActorVirtual52(actor, index);
            const auto delta = WrapSubtract(value, reread);
            const auto scaled = LegacyFtol(
                static_cast<double>(delta) *
                static_cast<double>(-0.699999988079071f));
            adjusted = WrapSubtract(reread, scaled);
        }
    }
    return Forward(callback, actor, index, adjusted, runtime);
}

}  // namespace pal4
