#include "pal4/combat_probability_calculator.h"

#include <bit>

namespace pal4 {
namespace {

float DatabaseFactor(std::uint32_t index,
                     ICombatProbabilityRuntime& runtime) {
    return runtime.DatabaseFactor(index);
}

std::int32_t SignedDifference(std::uint32_t left, std::uint32_t right) {
    return std::bit_cast<std::int32_t>(left - right);
}

double ClampUnit(double value) {
    if (value > 1.0)
        return 1.0;
    if (value < 0.0)
        return 0.0;
    return value;
}

float Roll(ICombatProbabilityRuntime& runtime) {
    return static_cast<float>(
        static_cast<double>(runtime.RandomNumber() % 1000u) * 0.001);
}

bool CheckIndexedThreshold(std::uint32_t context, std::uint32_t index,
                           std::uint32_t last_index,
                           std::uint32_t first_offset,
                           ICombatProbabilityRuntime& runtime) {
    const auto threshold = index <= last_index ?
        std::bit_cast<float>(runtime.ReadWord(
            context + first_offset + index * 4u)) :
        static_cast<float>(std::bit_cast<std::int32_t>(index));
    const auto roll = static_cast<double>(runtime.RandomNumber() % 1000u) *
                      0.001;
    return roll < threshold;
}

}  // namespace

float ICombatProbabilityRuntime::DatabaseFactor(std::uint32_t index) {
    return FloatArrayElement(GameDatabase() + 184u, index);
}

double CombatProbabilityCalculator::HitRate(
    std::uint32_t attacker, std::uint32_t defender,
    ICombatProbabilityRuntime& runtime) {
    const auto defender_stat = runtime.ReadWord(defender + 28u);
    const auto attacker_stat = runtime.ReadWord(attacker + 28u);
    const auto delta = SignedDifference(attacker_stat, defender_stat);
    const auto scaled_delta = static_cast<float>(
        static_cast<double>(DatabaseFactor(4u, runtime)) * delta);
    const auto base = DatabaseFactor(22u, runtime);
    const auto bonus = std::bit_cast<float>(
        runtime.ReadWord(attacker + 36u));
    return ClampUnit(static_cast<double>(base) + scaled_delta + bonus);
}

bool CombatProbabilityCalculator::CheckHit(
    std::uint32_t attacker, std::uint32_t defender,
    ICombatProbabilityRuntime& runtime) {
    const auto roll = Roll(runtime);
    return HitRate(attacker, defender, runtime) > roll;
}

double CombatProbabilityCalculator::DodgeRate(
    std::uint32_t attacker, std::uint32_t defender,
    ICombatProbabilityRuntime& runtime) {
    const auto defender_first = runtime.ReadWord(defender + 220u);
    const auto attacker_first = runtime.ReadWord(attacker + 220u);
    const auto first_scale = DatabaseFactor(20u, runtime);
    const auto defender_second = runtime.ReadWord(defender + 24u);
    const auto attacker_second = runtime.ReadWord(attacker + 24u);
    const auto first_delta = SignedDifference(attacker_first, defender_first);
    const auto first_term = static_cast<float>(
        static_cast<double>(first_scale) * first_delta);
    const auto second_scale = DatabaseFactor(21u, runtime);
    const auto second_delta = SignedDifference(attacker_second, defender_second);
    const auto second_term = static_cast<float>(
        static_cast<double>(second_scale) * second_delta);
    const auto base = DatabaseFactor(19u, runtime);
    const auto bonus = std::bit_cast<float>(
        runtime.ReadWord(attacker + 44u));
    return ClampUnit(static_cast<double>(base) + first_term +
                     second_term + bonus);
}

bool CombatProbabilityCalculator::CheckDodge(
    std::uint32_t attacker, std::uint32_t defender,
    float extra, ICombatProbabilityRuntime& runtime) {
    const auto roll = Roll(runtime);
    return DodgeRate(attacker, defender, runtime) + extra > roll;
}

double CombatProbabilityCalculator::CriticalRate(
    float numerator, float denominator,
    ICombatProbabilityRuntime& runtime) {
    const auto scale = DatabaseFactor(9u, runtime);
    const auto rate = static_cast<double>(scale) *
                      (static_cast<double>(numerator) / denominator);
    return rate > 1.0 ? 1.0 : rate;
}

bool CombatProbabilityCalculator::CheckCritical(
    float numerator, float denominator,
    ICombatProbabilityRuntime& runtime) {
    const auto roll = Roll(runtime);
    return CriticalRate(numerator, denominator, runtime) > roll;
}

double CombatProbabilityCalculator::FloatPropertyThreshold(
    [[maybe_unused]] std::uint32_t source, std::uint32_t target,
    ICombatProbabilityRuntime& runtime) {
    const auto property = std::bit_cast<float>(runtime.ReadWord(target + 40u));
    return static_cast<double>(property) + static_cast<double>(0.01f);
}

bool CombatProbabilityCalculator::CheckFloatProperty(
    std::uint32_t source, std::uint32_t target,
    ICombatProbabilityRuntime& runtime) {
    const auto roll = static_cast<double>(runtime.RandomNumber() % 1000u) *
                      0.001;
    return roll < FloatPropertyThreshold(source, target, runtime);
}

double CombatProbabilityCalculator::CriticalDamageMultiplier(
    std::uint32_t attacker, ICombatProbabilityRuntime& runtime) {
    const auto base = DatabaseFactor(10u, runtime);
    const auto stat = static_cast<float>(runtime.ActorStat(attacker, 5u));
    const auto scale = DatabaseFactor(11u, runtime);
    const auto raw = static_cast<double>(base) -
                     static_cast<double>(scale) * stat;
    return raw < 0.0 ? static_cast<double>(0.01f) : raw;
}

bool CombatProbabilityCalculator::CheckPrimaryRandomCondition(
    std::uint32_t context, std::uint32_t index,
    ICombatProbabilityRuntime& runtime) {
    return CheckIndexedThreshold(context, index, 5u, 144u, runtime);
}

bool CombatProbabilityCalculator::CheckSecondaryRandomCondition(
    std::uint32_t context, std::uint32_t index,
    ICombatProbabilityRuntime& runtime) {
    return CheckIndexedThreshold(context, index, 6u, 168u, runtime);
}

bool CombatProbabilityCalculator::CheckTertiaryRandomCondition(
    std::uint32_t context, std::uint32_t index,
    ICombatProbabilityRuntime& runtime) {
    return CheckIndexedThreshold(context, index, 1u, 196u, runtime);
}

}  // namespace pal4
