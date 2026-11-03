#pragma once

#include "pal4/combat_normal_damage_calculator.h"

#include <cstdint>

namespace pal4 {

class ICombatDamageActionRuntime : public ICombatNormalDamageRuntime {
public:
    // Actor virtual +52; formula parameters use the distinct virtual +48.
    [[nodiscard]] virtual std::int32_t CurrentStat(
        std::uint32_t actor, std::uint32_t stat) = 0;
    [[nodiscard]] virtual std::int64_t FloatToInt(float value) = 0;
    [[nodiscard]] virtual std::int32_t AddStat(
        std::uint32_t actor, std::uint32_t stat,
        std::int64_t delta) = 0;
    virtual void TargetFallback(std::uint32_t target,
                                std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t EnumerateTargets(
        std::uint32_t target, std::uint8_t selector) = 0;
    [[nodiscard]] virtual std::uint32_t VectorBegin(
        std::uint32_t vector) = 0;
    [[nodiscard]] virtual std::uint32_t VectorEnd(
        std::uint32_t vector) = 0;
    [[nodiscard]] virtual std::uint32_t CandidateStatus(
        std::uint32_t candidate) = 0;
    [[nodiscard]] virtual bool CheckCandidateActive(
        std::uint32_t candidate) = 0;
    [[nodiscard]] virtual std::uint32_t FreeVector(
        std::uint32_t vector) = 0;
};

class CombatDamageActionHandler final {
public:
    // IDA 0x5806d0.
    [[nodiscard]] static std::int32_t Skill(
        std::uint32_t action, std::uint32_t source,
        std::uint32_t target, ICombatDamageActionRuntime& runtime);
    // IDA 0x5808e0.
    [[nodiscard]] static std::int32_t Magic(
        std::uint32_t action, std::uint32_t source,
        std::uint32_t target, ICombatDamageActionRuntime& runtime);
};

}  // namespace pal4
