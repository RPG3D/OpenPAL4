#include "pal4/combat_damage_action_handler.h"

#include <bit>

namespace pal4 {
namespace {

float ReadFloat(std::uint32_t address, ICombatDamageActionRuntime& runtime) {
    return std::bit_cast<float>(runtime.ReadWord(address));
}

double Roll(ICombatDamageActionRuntime& runtime) {
    return static_cast<double>(runtime.RandomNumber() % 1000u) *
           static_cast<double>(0.001f);
}

class VectorRelease final {
public:
    VectorRelease(std::uint32_t vector, ICombatDamageActionRuntime& runtime)
        : vector_(vector), runtime_(runtime) {}
    ~VectorRelease() {
        if (!released_) static_cast<void>(runtime_.FreeVector(vector_));
    }
    std::uint32_t Release() {
        released_ = true;
        return runtime_.FreeVector(vector_);
    }

private:
    std::uint32_t vector_;
    ICombatDamageActionRuntime& runtime_;
    bool released_ = false;
};

void ApplyMain(const CombatDamagePair& damage,
               std::uint32_t source, std::uint32_t target,
               ICombatDamageActionRuntime& runtime) {
    const auto delta = runtime.FloatToInt(damage.main);
    if (delta != 0)
        static_cast<void>(runtime.AddStat(target, 7u, delta));
    else
        runtime.TargetFallback(target, source);
}

}  // namespace

std::int32_t CombatDamageActionHandler::Skill(
    std::uint32_t action, std::uint32_t source,
    std::uint32_t target, ICombatDamageActionRuntime& runtime) {
    if (Roll(runtime) > ReadFloat(action + 20u, runtime)) return 0;

    const auto element = static_cast<std::uint32_t>(runtime.FloatToInt(
        ReadFloat(action + 36u, runtime)));
    const auto multiplier = ReadFloat(action + 32u, runtime);
    CombatDamagePair primary;
    static_cast<void>(CombatNormalDamageCalculator::CalculateSkill(
        primary, source, target, element, multiplier, runtime));
    ApplyMain(primary, source, target, runtime);

    if (!(Roll(runtime) < ReadFloat(action + 24u, runtime)))
        return runtime.AddStat(source, 7u,
                               runtime.FloatToInt(primary.secondary));

    const auto vector = runtime.EnumerateTargets(
        target, static_cast<std::uint8_t>(action));
    VectorRelease release(vector, runtime);
    auto accumulated = primary.secondary;
    const auto splash_multiplier = static_cast<float>(
        ReadFloat(action + 28u, runtime) * multiplier);
    for (std::uint32_t index = 0u;; ++index) {
        const auto begin = runtime.VectorBegin(vector);
        if (begin == 0u || index >=
                (runtime.VectorEnd(vector) - begin) / 4u)
            break;
        const auto candidate = runtime.ReadWord(begin + index * 4u);
        if (candidate == 0u || runtime.CandidateStatus(candidate) == 4u ||
            !runtime.CheckCandidateActive(candidate))
            continue;
        CombatDamagePair splash;
        static_cast<void>(CombatNormalDamageCalculator::CalculateSkill(
            splash, source, candidate, element, splash_multiplier, runtime));
        ApplyMain(splash, source, candidate, runtime);
        accumulated = static_cast<float>(accumulated + splash.secondary);
    }
    static_cast<void>(runtime.AddStat(source, 7u,
                                      runtime.FloatToInt(accumulated)));
    return std::bit_cast<std::int32_t>(release.Release());
}

std::int32_t CombatDamageActionHandler::Magic(
    std::uint32_t action, std::uint32_t source,
    std::uint32_t target, ICombatDamageActionRuntime& runtime) {
    if (Roll(runtime) > ReadFloat(action + 20u, runtime)) return 0;

    CombatDamagePair primary;
    static_cast<void>(CombatNormalDamageCalculator::CalculateMagic(
        primary, source, target, ReadFloat(action + 32u, runtime),
        runtime));
    const auto main = runtime.FloatToInt(primary.main);
    if (main != 0) {
        static_cast<void>(runtime.AddStat(target, 7u, main));
        if (runtime.CurrentStat(target, 7u) > 0)
            static_cast<void>(runtime.AddStat(source, 7u,
                runtime.FloatToInt(primary.secondary)));
    } else {
        runtime.TargetFallback(target, source);
    }

    if (!(Roll(runtime) < ReadFloat(action + 24u, runtime))) return 0;

    const auto splash = static_cast<float>(
        primary.main * ReadFloat(action + 28u, runtime));
    const auto vector = runtime.EnumerateTargets(
        target, static_cast<std::uint8_t>(action));
    VectorRelease release(vector, runtime);
    for (std::uint32_t index = 0u;; ++index) {
        const auto begin = runtime.VectorBegin(vector);
        if (begin == 0u || index >=
                (runtime.VectorEnd(vector) - begin) / 4u)
            break;
        const auto candidate = runtime.ReadWord(begin + index * 4u);
        if (candidate == 0u || runtime.CandidateStatus(candidate) == 4u ||
            !runtime.CheckCandidateActive(candidate))
            continue;
        static_cast<void>(runtime.AddStat(candidate, 7u,
                                          runtime.FloatToInt(splash)));
    }
    return std::bit_cast<std::int32_t>(release.Release());
}

}  // namespace pal4
