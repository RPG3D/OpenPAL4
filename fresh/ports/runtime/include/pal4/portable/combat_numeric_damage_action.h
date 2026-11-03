#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace pal4::portable {

class CombatFormulaCatalog;
}

namespace pal4 {
class RandomNumberGenerator;
}

namespace pal4::portable {

using CombatNumericActorHandle = std::uint64_t;

struct CombatNumericSkillAction final {
    float primary_chance{};
    float secondary_chance{};
    float splash_multiplier{};
    float skill_multiplier{};
    float element{};
    std::uint8_t target_selector{};
};

// Typed access to the host's existing Combat actor/property owners. AddCurrentStat
// must preserve the actor84/property184 health and notification path.
class ICombatNumericActorAccess {
public:
    virtual ~ICombatNumericActorAccess() = default;
    virtual void ValidateActor(CombatNumericActorHandle actor) const = 0;
    [[nodiscard]] virtual std::int32_t Stat(
        CombatNumericActorHandle actor, std::uint32_t index) = 0;
    // Current-attribute chain (actor virtual +52), distinct from Stat's
    // formula-property chain (actor virtual +48).
    [[nodiscard]] virtual std::int32_t CurrentStat(
        CombatNumericActorHandle actor, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::int32_t Bonus(
        CombatNumericActorHandle actor, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::int32_t Element(
        CombatNumericActorHandle actor, std::uint32_t index) = 0;
    [[nodiscard]] virtual double DamageFactor(
        CombatNumericActorHandle actor, std::uint32_t mode) = 0;
    [[nodiscard]] virtual double TargetFactor(
        CombatNumericActorHandle actor, std::uint32_t element) = 0;
    [[nodiscard]] virtual double SecondaryFactor(
        CombatNumericActorHandle actor, std::uint32_t mode) = 0;
    [[nodiscard]] virtual double Reduction(
        CombatNumericActorHandle actor, std::uint32_t mode) = 0;
    [[nodiscard]] virtual bool HalfDamage(
        CombatNumericActorHandle actor) = 0;
    [[nodiscard]] virtual std::int32_t AddCurrentStat(
        CombatNumericActorHandle actor, std::uint32_t stat,
        std::int64_t delta) = 0;
    virtual void TargetFallback(CombatNumericActorHandle target,
                                CombatNumericActorHandle source) = 0;
    [[nodiscard]] virtual std::vector<CombatNumericActorHandle> RelatedTargets(
        CombatNumericActorHandle target, std::uint8_t selector) = 0;
    [[nodiscard]] virtual std::uint32_t CombatMode(
        CombatNumericActorHandle actor) = 0;
    [[nodiscard]] virtual bool IsNotStateThree(
        CombatNumericActorHandle actor) = 0;
};

// Runs shared 9116 damage against registered host actor owners. Formula values
// and RNG remain owned by their original catalogs; no actor memory is copied.
class CombatNumericDamageAction final {
public:
    CombatNumericDamageAction(const CombatFormulaCatalog&,
                              RandomNumberGenerator&,
                              ICombatNumericActorAccess&);
    ~CombatNumericDamageAction();
    CombatNumericDamageAction(const CombatNumericDamageAction&) = delete;
    CombatNumericDamageAction& operator=(const CombatNumericDamageAction&) = delete;

    [[nodiscard]] std::int32_t ApplySkill(
        const CombatNumericSkillAction& action,
        CombatNumericActorHandle source,
        CombatNumericActorHandle target);
    // Lifetime total. Snapshot before/after ApplySkill to measure one call.
    [[nodiscard]] std::uint32_t RandomDraws() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace pal4::portable
