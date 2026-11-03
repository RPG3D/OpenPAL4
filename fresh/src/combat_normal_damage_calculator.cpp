#include "pal4/combat_normal_damage_calculator.h"

#include "pal4/combat_event_property.h"

#include <cstdint>
#include <utility>

namespace pal4 {
namespace {

float DatabaseFactor(std::uint32_t index,
                     ICombatNormalDamageRuntime& runtime) {
    return runtime.DatabaseFactor(index);
}

double RandomInterpolation(float first, float second,
                           ICombatNormalDamageRuntime& runtime) {
    if (first > second)
        std::swap(first, second);
    const auto sample = runtime.RandomNumber() % 1000u;
    return static_cast<double>(sample) * 0.001 *
           (static_cast<double>(second) - first) + first;
}

float RandomFactor(float first, float second,
                   ICombatNormalDamageRuntime& runtime) {
    return static_cast<float>(RandomInterpolation(first, second, runtime));
}

float ElementalTotal(std::uint32_t attacker, std::uint32_t defender,
                     ICombatNormalDamageRuntime& runtime) {
    float total = 0.0f;
    for (std::uint32_t index = 0u; index < 5u; ++index) {
        const auto stat = static_cast<float>(
            runtime.ActorStat(attacker, 7u));
        const auto bonus = static_cast<float>(
            static_cast<double>(runtime.ActorBonus(attacker, index + 1u)) *
            static_cast<double>(0.01f));
        const auto database_element = DatabaseFactor(5u, runtime);
        const auto attacker_element = static_cast<float>(
            runtime.ActorElement(attacker, index));
        const auto defender_element = static_cast<float>(
            runtime.ActorElement(defender, CombatDamageState::Next(index)));
        const auto lower = DatabaseFactor(6u, runtime);
        const auto upper = DatabaseFactor(7u, runtime);
        const auto element_random = RandomFactor(lower, upper, runtime);
        const auto actor_factor = runtime.ActorDamageFactor(attacker, 0u);
        const auto element_bonus =
            (static_cast<double>(defender_element) + attacker_element) *
                static_cast<double>(0.01f) + 1.0;
        const auto contribution = actor_factor *
            (element_bonus *
             (static_cast<double>(database_element) * bonus * stat) *
             element_random);
        total = static_cast<float>(total + contribution);
    }
    return total;
}

}  // namespace

float ICombatNormalDamageRuntime::DatabaseFactor(std::uint32_t index) {
    return FloatArrayElement(GameDatabase() + 184u,index);
}

CombatDamagePair* CombatNormalDamageCalculator::Calculate(
    CombatDamagePair& output, std::uint32_t attacker,
    std::uint32_t defender, bool skip_half,
    ICombatNormalDamageRuntime& runtime) {
    const auto attack_stat = static_cast<float>(
        runtime.ActorStat(attacker, 3u));
    const auto scaled_stat = static_cast<float>(
        static_cast<double>(DatabaseFactor(0u, runtime)) * attack_stat);
    const auto defense_stat = runtime.ActorStat(defender, 4u);
    auto base = static_cast<float>(
        static_cast<double>(scaled_stat) - defense_stat);
    if (base < 0.0f)
        base = 0.0f;

    const auto first = DatabaseFactor(1u, runtime);
    const auto second = DatabaseFactor(2u, runtime);
    const auto random_factor = RandomFactor(first, second, runtime);
    const auto base_product = static_cast<float>(
        static_cast<double>(random_factor) * base);
    const auto actor_product = static_cast<float>(
        runtime.ActorDamageFactor(attacker, 1u) * base_product);
    const auto target_element = runtime.ReadWord(defender + 340u);
    const auto adjusted_product = static_cast<float>(
        runtime.ActorTargetFactor(attacker, target_element) * actor_product);
    const auto secondary = static_cast<float>(
        runtime.DefenderSecondaryFactor(defender, 0u) * adjusted_product);
    auto main = static_cast<float>(
        (1.0 - runtime.DefenderReduction(defender, 0u)) * adjusted_product);
    if (!skip_half && runtime.ReadWord(defender + 660u) == 1u)
        main = static_cast<float>(
            runtime.HalfValue(attacker, defender) * main);
    main = static_cast<float>(
        static_cast<double>(runtime.ActorBonus(attacker, 0u)) + main);

    const auto elemental_total = ElementalTotal(attacker, defender, runtime);

    output.main = -(elemental_total + main);
    output.secondary = -secondary;
    return &output;
}

CombatDamagePair* CombatNormalDamageCalculator::CalculateExtended(
    CombatDamagePair& output, std::uint32_t attacker,
    std::uint32_t defender, bool skip_half,
    ICombatNormalDamageRuntime& runtime) {
    const auto scaling = DatabaseFactor(0u, runtime);
    const auto attack_stat = runtime.ActorStat(attacker, 3u);
    const auto scaled_stat = static_cast<float>(
        static_cast<double>(attack_stat) * scaling);
    const auto defense_stat = runtime.ActorStat(defender, 4u);
    auto base = static_cast<float>(
        static_cast<double>(scaled_stat) - defense_stat);
    if (base < 0.0f)
        base = 0.0f;

    const auto database_product = static_cast<float>(
        static_cast<double>(DatabaseFactor(3u, runtime)) * base);
    const auto actor_product = static_cast<float>(
        runtime.ActorDamageFactor(attacker, 1u) * database_product);
    const auto target_element = runtime.ReadWord(defender + 340u);
    const auto adjusted_product = static_cast<float>(
        runtime.ActorTargetFactor(attacker, target_element) * actor_product);
    const auto secondary = static_cast<float>(
        runtime.DefenderSecondaryFactor(defender, 0u) * adjusted_product);
    auto main = static_cast<float>(
        (1.0 - runtime.DefenderReduction(defender, 0u)) * adjusted_product);
    if (!skip_half && runtime.ReadWord(defender + 660u) == 1u)
        main = static_cast<float>(
            runtime.HalfValue(attacker, defender) * main);
    main = static_cast<float>(
        static_cast<double>(runtime.ActorBonus(attacker, 0u)) + main);

    const auto elemental_total = ElementalTotal(attacker, defender, runtime);
    output.main = -(elemental_total + main);
    output.secondary = -secondary;
    return &output;
}

CombatDamagePair* CombatNormalDamageCalculator::CalculateSkill(
    CombatDamagePair& output, std::uint32_t attacker,
    std::uint32_t defender, std::uint32_t element_bits,
    float skill_multiplier, ICombatNormalDamageRuntime& runtime) {
    const auto stat = runtime.ActorStat(attacker, 7u);
    const auto scaled_stat = static_cast<float>(
        static_cast<double>(stat) * skill_multiplier);
    const auto database_product = static_cast<float>(
        static_cast<double>(DatabaseFactor(5u, runtime)) * scaled_stat);
    const auto attacker_element = static_cast<float>(
        runtime.ActorElement(attacker, element_bits));
    const auto defender_element = runtime.ActorElement(
        defender, CombatDamageState::Next(element_bits));
    const auto element_product = static_cast<float>(
        ((static_cast<double>(defender_element) + attacker_element) *
              static_cast<double>(0.01f) + 1.0) * database_product);
    const auto lower = DatabaseFactor(6u, runtime);
    const auto upper = DatabaseFactor(7u, runtime);
    auto random_product = static_cast<float>(
        RandomInterpolation(lower, upper, runtime) * element_product);
    if (random_product < 0.0f)
        random_product = 0.0f;
    const auto actor_product = static_cast<float>(
        runtime.ActorDamageFactor(attacker, 0u) * random_product);
    const auto target_slot = element_bits + 1u;
    const auto reduction = static_cast<float>(
        1.0 - runtime.DefenderReduction(defender, target_slot));
    const auto secondary = static_cast<float>(
        runtime.DefenderSecondaryFactor(defender, target_slot) * actor_product);
    auto main = static_cast<float>(
        static_cast<double>(reduction) * actor_product);
    if (runtime.ReadWord(defender + 660u) == 1u)
        main = static_cast<float>(
            runtime.HalfValue(attacker, defender) * main);
    output.main = -main;
    output.secondary = -secondary;
    return &output;
}

CombatDamagePair* CombatNormalDamageCalculator::CalculateMagic(
    CombatDamagePair& output, std::uint32_t attacker,
    std::uint32_t defender, float magic_multiplier,
    ICombatNormalDamageRuntime& runtime) {
    const auto attack_stat = static_cast<float>(
        runtime.ActorStat(attacker, 3u));
    const auto scaled_stat = static_cast<float>(
        static_cast<double>(DatabaseFactor(0u, runtime)) * attack_stat);
    const auto defense_stat = runtime.ActorStat(defender, 4u);
    auto base = static_cast<float>(
        static_cast<double>(scaled_stat) - defense_stat);
    if (base < 0.0f)
        base = 0.0f;

    const auto magic_product = static_cast<float>(
        static_cast<double>(base) * magic_multiplier);
    const auto actor_product = static_cast<float>(
        runtime.ActorDamageFactor(attacker, 1u) * magic_product);
    const auto target_element = runtime.ReadWord(defender + 340u);
    const auto adjusted_product = static_cast<float>(
        runtime.ActorTargetFactor(attacker, target_element) * actor_product);
    const auto secondary = static_cast<float>(
        runtime.DefenderSecondaryFactor(defender, 0u) * adjusted_product);
    auto main = static_cast<float>(
        (1.0 - runtime.DefenderReduction(defender, 0u)) * adjusted_product);
    if (runtime.ReadWord(defender + 660u) == 1u)
        main = static_cast<float>(
            runtime.HalfValue(attacker, defender) * main);
    const auto defender_bonus = runtime.ActorBonus(defender, 0u);
    output.main = -static_cast<float>(
        static_cast<double>(defender_bonus) + main);
    output.secondary = -secondary;
    return &output;
}

double CombatDamageConstants::HalfValue() {
    return 0.5;
}

}  // namespace pal4
