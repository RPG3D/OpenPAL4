#pragma once

#include <cstdint>

namespace pal4 {

class ICombatProbabilityRuntime {
public:
    virtual ~ICombatProbabilityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual float FloatArrayElement(
        std::uint32_t table, std::uint32_t index) = 0;
    // Native typed catalogs need no legacy GameDB prefix/vector image.
    [[nodiscard]] virtual float DatabaseFactor(std::uint32_t index);
    [[nodiscard]] virtual std::uint32_t RandomNumber() = 0;
    [[nodiscard]] virtual std::int32_t ActorStat(
        std::uint32_t actor, std::uint32_t index) = 0;
};

class CombatProbabilityCalculator final {
public:
    // IDA 0x57d7e0.
    [[nodiscard]] static double HitRate(
        std::uint32_t attacker, std::uint32_t defender,
        ICombatProbabilityRuntime& runtime);
    // IDA 0x57d870.
    [[nodiscard]] static bool CheckHit(
        std::uint32_t attacker, std::uint32_t defender,
        ICombatProbabilityRuntime& runtime);
    // IDA 0x57d8d0.
    [[nodiscard]] static double DodgeRate(
        std::uint32_t attacker, std::uint32_t defender,
        ICombatProbabilityRuntime& runtime);
    // IDA 0x57d990.
    [[nodiscard]] static bool CheckDodge(
        std::uint32_t attacker, std::uint32_t defender,
        float extra, ICombatProbabilityRuntime& runtime);
    // IDA 0x57d9f0.
    [[nodiscard]] static double CriticalRate(
        float numerator, float denominator,
        ICombatProbabilityRuntime& runtime);
    // IDA 0x57da30.
    [[nodiscard]] static bool CheckCritical(
        float numerator, float denominator,
        ICombatProbabilityRuntime& runtime);
    // IDA 0x57da90.
    [[nodiscard]] static double FloatPropertyThreshold(
        std::uint32_t source, std::uint32_t target,
        ICombatProbabilityRuntime& runtime);
    // IDA 0x57daa0.
    [[nodiscard]] static bool CheckFloatProperty(
        std::uint32_t source, std::uint32_t target,
        ICombatProbabilityRuntime& runtime);
    // IDA 0x57daf0.
    [[nodiscard]] static double CriticalDamageMultiplier(
        std::uint32_t attacker, ICombatProbabilityRuntime& runtime);
    // IDA 0x57dd10.
    [[nodiscard]] static bool CheckPrimaryRandomCondition(
        std::uint32_t context, std::uint32_t index,
        ICombatProbabilityRuntime& runtime);
    // IDA 0x57dd70.
    [[nodiscard]] static bool CheckSecondaryRandomCondition(
        std::uint32_t context, std::uint32_t index,
        ICombatProbabilityRuntime& runtime);
    // IDA 0x57ddd0.
    [[nodiscard]] static bool CheckTertiaryRandomCondition(
        std::uint32_t context, std::uint32_t index,
        ICombatProbabilityRuntime& runtime);
};

}  // namespace pal4
