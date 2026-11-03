#pragma once

#include <cstdint>

namespace pal4 {

struct CombatDamagePair {
    float main = 0.0f;
    float secondary = 0.0f;
};

class ICombatNormalDamageRuntime {
public:
    virtual ~ICombatNormalDamageRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t ActorStat(
        std::uint32_t actor, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::int32_t ActorBonus(
        std::uint32_t actor, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::int32_t ActorElement(
        std::uint32_t actor, std::uint32_t index) = 0;
    [[nodiscard]] virtual double ActorDamageFactor(
        std::uint32_t actor, std::uint32_t mode) = 0;
    [[nodiscard]] virtual double ActorTargetFactor(
        std::uint32_t actor, std::uint32_t target_element) = 0;
    [[nodiscard]] virtual double DefenderSecondaryFactor(
        std::uint32_t defender, std::uint32_t mode) = 0;
    [[nodiscard]] virtual double DefenderReduction(
        std::uint32_t defender, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual float FloatArrayElement(
        std::uint32_t table, std::uint32_t index) = 0;
    // Typed native catalogs need no legacy GameDB prefix/vector image.
    [[nodiscard]] virtual float DatabaseFactor(std::uint32_t index);
    [[nodiscard]] virtual std::uint32_t RandomNumber() = 0;
    [[nodiscard]] virtual double HalfValue(
        std::uint32_t attacker, std::uint32_t defender) = 0;
};

class CombatNormalDamageCalculator final {
public:
    // IDA 0x57cf80.
    [[nodiscard]] static CombatDamagePair* Calculate(
        CombatDamagePair& output, std::uint32_t attacker,
        std::uint32_t defender, bool skip_half,
        ICombatNormalDamageRuntime& runtime);
    // IDA 0x57d280.
    [[nodiscard]] static CombatDamagePair* CalculateExtended(
        CombatDamagePair& output, std::uint32_t attacker,
        std::uint32_t defender, bool skip_half,
        ICombatNormalDamageRuntime& runtime);
    // IDA 0x57d520: element_bits is the raw 32-bit fourth argument.
    [[nodiscard]] static CombatDamagePair* CalculateSkill(
        CombatDamagePair& output, std::uint32_t attacker,
        std::uint32_t defender, std::uint32_t element_bits,
        float skill_multiplier, ICombatNormalDamageRuntime& runtime);
    // IDA 0x57d6d0.
    [[nodiscard]] static CombatDamagePair* CalculateMagic(
        CombatDamagePair& output, std::uint32_t attacker,
        std::uint32_t defender, float magic_multiplier,
        ICombatNormalDamageRuntime& runtime);
};

class CombatDamageConstants final {
public:
    // IDA 0x57db60.
    [[nodiscard]] static double HalfValue();
};

}  // namespace pal4
