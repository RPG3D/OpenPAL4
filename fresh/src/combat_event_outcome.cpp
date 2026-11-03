#include "pal4/combat_event_outcome.h"

#include <bit>
#include <cmath>

namespace pal4 {
namespace {

float ReadFloat(std::uint32_t address, ICombatEventOutcomeRuntime& runtime) {
    return std::bit_cast<float>(runtime.ReadWord(address));
}

double Roll(ICombatEventOutcomeRuntime& runtime) {
    return static_cast<double>(runtime.RandomNumber() % 1000u) *
           static_cast<double>(0.001f);
}

class VectorRelease final {
public:
    VectorRelease(std::uint32_t vector, ICombatEventOutcomeRuntime& runtime)
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
    ICombatEventOutcomeRuntime& runtime_;
    bool released_ = false;
};

std::uint32_t ProcessTargets(std::uint32_t slot, std::uint32_t record,
                             std::uint32_t target,
                             ICombatEventOutcomeRuntime& runtime) {
    const auto vector = runtime.EnumerateTargets(
        target, static_cast<std::uint8_t>(record));
    VectorRelease release(vector, runtime);
    for (std::uint32_t index = 0;; ++index) {
        const auto begin = runtime.VectorBegin(vector);
        if (begin == 0u || index >=
                (runtime.VectorEnd(vector) - begin) / 4u)
            break;
        const auto candidate = runtime.ReadWord(begin + index * 4u);
        if (candidate != 0u && runtime.CandidateStatus(candidate) != 4u)
            runtime.ProcessObject(slot, candidate);
    }
    return release.Release();
}

}  // namespace

std::uint8_t CombatEventOutcome::Process(
    std::uint32_t record, [[maybe_unused]] std::uint32_t unused_second,
    [[maybe_unused]] std::uint32_t unused_third, std::uint32_t target,
    ICombatEventOutcomeRuntime& runtime) {
    const auto first_roll = Roll(runtime);
    const auto first_threshold = ReadFloat(record + 20u, runtime);
    if (first_roll > first_threshold) return 0u;

    const auto primary_second = runtime.FloatToInt(
        ReadFloat(record + 40u, runtime));
    const auto primary_scale = ReadFloat(record + 36u, runtime);
    const auto primary_first = runtime.FloatToInt(
        ReadFloat(record + 32u, runtime));
    runtime.SetEventData(0u, target, primary_first, primary_scale,
                         primary_second);

    const auto secondary_second = runtime.FloatToInt(
        ReadFloat(record + 52u, runtime));
    const auto secondary_scale = ReadFloat(record + 48u, runtime);
    const auto secondary_first = runtime.FloatToInt(
        ReadFloat(record + 44u, runtime));
    runtime.SetEventData(1u, target, secondary_first, secondary_scale,
                         secondary_second);
    runtime.ProcessObject(0u, target);
    runtime.ProcessObject(1u, target);

    const auto second_roll = Roll(runtime);
    const auto second_threshold = ReadFloat(record + 24u, runtime);
    if (!(second_roll < second_threshold)) return 0u;

    if (runtime.Matches(0u, runtime.ReadWord(record + 28u)))
        static_cast<void>(ProcessTargets(0u, record, target, runtime));
    if (!runtime.Matches(1u, runtime.ReadWord(record + 28u))) return 0u;
    return static_cast<std::uint8_t>(
        ProcessTargets(1u, record, target, runtime));
}

std::int32_t CombatEventOutcome::ProcessMultiTarget(
    std::uint32_t action, std::uint32_t target,
    ICombatEventOutcomeRuntime& runtime) {
    const auto roll = Roll(runtime);
    if (roll > ReadFloat(action + 20u, runtime)) return 0;

    const auto vector = runtime.EnumerateTargets(
        target, static_cast<std::uint8_t>(action));
    VectorRelease release(vector, runtime);
    for (std::uint32_t index = 0;; ++index) {
        const auto begin = runtime.VectorBegin(vector);
        if (begin == 0u || index >=
                (runtime.VectorEnd(vector) - begin) / 4u)
            break;
        const auto candidate = runtime.ReadWord(begin + index * 4u);
        if (candidate == 0u) continue;

        const auto first_second = runtime.FloatToInt(
            ReadFloat(action + 32u, runtime));
        const auto first_scale = ReadFloat(action + 28u, runtime);
        const auto first_first = runtime.FloatToInt(
            ReadFloat(action + 24u, runtime));
        runtime.SetEventData(0u, candidate,
                             first_first, first_scale, first_second);

        const auto second_second = runtime.FloatToInt(
            ReadFloat(action + 44u, runtime));
        const auto second_scale = ReadFloat(action + 40u, runtime);
        const auto second_first = runtime.FloatToInt(
            ReadFloat(action + 36u, runtime));
        runtime.SetEventData(1u, candidate,
                             second_first, second_scale, second_second);
        runtime.ProcessObject(0u, candidate);
        runtime.ProcessObject(1u, candidate);
    }
    return std::bit_cast<std::int32_t>(release.Release());
}

std::int32_t CombatEventOutcome::ProcessMultiEffect(
    [[maybe_unused]] std::uint32_t unused_register,
    std::uint32_t action, std::uint32_t source,
    std::uint32_t target, ICombatMultiEffectRuntime& runtime) {
    const auto vector = runtime.EnumerateTargets(
        target, static_cast<std::uint8_t>(target));
    VectorRelease release(vector, runtime);
    const auto begin = runtime.VectorBegin(vector);
    std::uint32_t active = 0u;
    if (begin != runtime.VectorEnd(vector)) {
        for (auto at = begin; at != runtime.VectorEnd(vector); at += 4u)
            if (runtime.CheckCandidateActive(runtime.ReadWord(at)))
                ++active;
    }
    if (static_cast<double>(active) > ReadFloat(action + 20u, runtime))
        return std::bit_cast<std::int32_t>(release.Release());

    for (std::uint32_t pair = 0u; pair < 4u; ++pair) {
        const auto offset = 24u + pair * 8u;
        const auto second = static_cast<std::int32_t>(runtime.FloatToInt(
            ReadFloat(action + offset + 4u, runtime)));
        const auto first = static_cast<std::int32_t>(runtime.FloatToInt(
            ReadFloat(action + offset, runtime)));
        runtime.DispatchPair(source, first, second);
    }
    return std::bit_cast<std::int32_t>(release.Release());
}

}  // namespace pal4
