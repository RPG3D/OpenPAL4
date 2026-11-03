#include "pal4/combat_action_effects.h"

#include <bit>

namespace pal4 {
namespace {

template<class Runtime>
float ReadFloat(std::uint32_t address, Runtime& runtime) {
    return std::bit_cast<float>(runtime.ReadWord(address));
}

template<class Runtime>
bool PassesRoll(std::uint32_t action, Runtime& runtime) {
    const auto roll = static_cast<double>(runtime.RandomNumber() % 1000u) *
                      static_cast<double>(0.001f);
    return !(roll > ReadFloat(action + 20u, runtime));
}

template<class Runtime>
std::int32_t FallbackOrGuard(std::uint32_t source,
                             std::uint32_t guard_offset,
                             Runtime& runtime) {
    const auto guard = runtime.ReadWord(source + guard_offset);
    return guard ? std::bit_cast<std::int32_t>(guard)
                 : runtime.Fallback(source);
}

}  // namespace

std::int32_t CombatActionEffects::HandlePropertyType(
    std::uint32_t action, std::uint32_t target,
    ICombatScalarEffectsRuntime& runtime) {
    const auto kind = runtime.FloatToInt(ReadFloat(action + 24u, runtime));
    if (kind == 1 || kind == 3)
        return runtime.SetProperty(target, kind == 1 ? 2u : 3u,
                                   runtime.ReadWord(action + 20u));
    return static_cast<std::int32_t>(kind);
}

std::int32_t CombatActionEffects::ProcessStatTransfer(
    std::uint32_t action, std::uint32_t source,
    std::uint32_t context, std::uint32_t target,
    ICombatStatTransferRuntime& runtime) {
    if (!PassesRoll(action, runtime)) return 0;
    if (runtime.CheckFurther(target + 340u, 0u))
        return runtime.DispatchAction(target, context, action, 0u);

    const auto first_scale = ReadFloat(action + 24u, runtime);
    const auto first_product = static_cast<float>(
        static_cast<double>(runtime.GetStat(target, 0u)) * first_scale);
    static_cast<void>(runtime.AddStat(source, 7u,
        runtime.FloatToInt(first_product)));
    static_cast<void>(runtime.AddStat(target, 7u,
        runtime.FloatToInt(-first_product)));

    const auto second_scale = ReadFloat(action + 28u, runtime);
    const auto second_product = static_cast<float>(
        static_cast<double>(runtime.GetStat(target, 2u)) * second_scale);
    static_cast<void>(runtime.AddStat(source, 9u,
        runtime.FloatToInt(second_product)));
    return runtime.AddStat(target, 9u,
                           runtime.FloatToInt(-second_product));
}

std::int32_t CombatActionEffects::ConsumeSlotsZeroOne(
    std::uint32_t action, std::uint32_t source,
    std::uint32_t target, ICombatSlotConsumeRuntime& runtime) {
    if (!PassesRoll(action, runtime))
        return FallbackOrGuard(source, 712u, runtime);

    const auto first = runtime.GetSlot(target, 0u);
    const auto second = runtime.GetSlot(target, 1u);
    runtime.SetSlot(target, 0u, 0);
    runtime.SetSlot(target, 1u, 0);
    runtime.WriteWord(target + 208u, 0u);
    runtime.WriteWord(target + 212u, 0u);
    if (first == 0) return FallbackOrGuard(source, 712u, runtime);

    const auto result = runtime.TransferSlots(source, first, second);
    runtime.WriteWord(source + 708u, std::bit_cast<std::uint32_t>(first));
    return result;
}

std::int32_t CombatActionEffects::ConsumeSlotTwo(
    std::uint32_t action, std::uint32_t source,
    std::uint32_t target, ICombatSlotConsumeRuntime& runtime) {
    if (!PassesRoll(action, runtime))
        return FallbackOrGuard(source, 708u, runtime);

    const auto value = runtime.GetSlot(target, 2u);
    runtime.SetSlot(target, 2u, 0);
    runtime.WriteWord(target + 216u, 0u);
    if (value <= 0) return FallbackOrGuard(source, 708u, runtime);

    const auto result = runtime.TransferPositiveSlot(source, value);
    runtime.WriteWord(source + 712u, std::bit_cast<std::uint32_t>(value));
    return result;
}

std::int32_t CombatActionEffects::HandlePropertyTypeFour(
    std::uint32_t action, std::uint32_t target,
    ICombatScalarEffectsRuntime& runtime) {
    return runtime.SetProperty(target, 4u, runtime.ReadWord(action + 20u));
}

std::int32_t CombatActionEffects::HandleStatDeltaThree(
    std::uint32_t action, std::uint32_t target,
    ICombatActionEffectsRuntime& runtime) {
    return runtime.AddStat(target, 3u,
                           runtime.FloatToInt(ReadFloat(action + 20u,
                                                        runtime)));
}

std::int32_t CombatActionEffects::HandlePropertyTypeZero(
    std::uint32_t action, std::uint32_t target,
    ICombatScalarEffectsRuntime& runtime) {
    return runtime.SetProperty(target, 0u, runtime.ReadWord(action + 20u));
}

std::int32_t CombatActionEffects::HandlePropertyTypeOne(
    std::uint32_t action, std::uint32_t target,
    ICombatScalarEffectsRuntime& runtime) {
    return runtime.SetProperty(target, 1u, runtime.ReadWord(action + 20u));
}

std::int32_t CombatActionEffects::DispatchIndexed(
    std::uint32_t action, std::uint32_t target,
    ICombatIndexedEffectsRuntime& runtime) {
    const auto index = static_cast<std::int32_t>(
        runtime.FloatToInt(std::bit_cast<float>(runtime.ReadWord(action + 20u))));
    if (index < 50 || index > 56) return index;
    const auto event = runtime.LookupEvent(index);
    if (event == 0u) return 0;
    return runtime.DispatchEvent(event, target,
                                 runtime.ReadWord(action + 24u));
}

bool CombatActionEffects::TryScalarCallback(std::uint32_t callback,
    std::uint32_t action,std::uint32_t target,ICombatScalarEffectsRuntime& runtime) {
    switch(callback) {
    case 0x5800b0u: (void)HandlePropertyType(action,target,runtime); return true;
    case 0x580390u: (void)HandlePropertyTypeFour(action,target,runtime); return true;
    case 0x580510u: (void)HandlePropertyTypeZero(action,target,runtime); return true;
    case 0x580530u: (void)HandlePropertyTypeOne(action,target,runtime); return true;
    default: return false;
    }
}

}  // namespace pal4
