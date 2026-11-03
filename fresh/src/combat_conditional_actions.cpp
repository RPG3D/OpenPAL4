#include "pal4/combat_conditional_actions.h"

#include <bit>
#include <cmath>
#include <stdexcept>

namespace pal4 {
namespace {

float ReadFloat(std::uint32_t address, ICombatConditionalActionsRuntime& runtime) {
    return std::bit_cast<float>(runtime.ReadWord(address));
}

struct GateResult {
    bool accepted;
    std::int32_t rejected_result;
};

GateResult CheckGate(std::uint32_t action, std::uint32_t actor,
                     ICombatConditionalActionsRuntime& runtime) {
    const auto mode = runtime.FloatToInt(ReadFloat(action + 20u, runtime));
    if (mode == 0) {
        const auto stat = runtime.ActorStat(actor, 7u);
        const auto base = runtime.ActorBase(actor, 0u);
        const auto ratio = static_cast<double>(stat) / static_cast<double>(base);
        const auto threshold = ReadFloat(action + 24u, runtime);
        return {ratio <= threshold || std::isnan(ratio) || std::isnan(threshold), 0};
    }
    if (mode == 1) {
        const auto counter = runtime.ActorCounter(actor);
        const auto threshold = runtime.FloatToInt(ReadFloat(action + 24u, runtime));
        return {counter == threshold, threshold};
    }
    const auto threshold = ReadFloat(action + 24u, runtime);
    const auto divisor = threshold != 0.0f || std::isnan(threshold) ? 1 : 0;
    const auto counter = runtime.ActorCounter(actor);
    if (divisor == 0) throw std::domain_error("native conditional action divides by zero");
    return {true, counter};
}

class IdRelease final {
public:
    IdRelease(const std::vector<std::int32_t>& ids,
              ICombatConditionalActionsRuntime& runtime)
        : ids_(ids), runtime_(runtime) {}
    ~IdRelease() {
        if (!released_) static_cast<void>(runtime_.ReleaseIds(ids_));
    }
    std::int32_t Release() {
        released_ = true;
        return runtime_.ReleaseIds(ids_);
    }

private:
    const std::vector<std::int32_t>& ids_;
    ICombatConditionalActionsRuntime& runtime_;
    bool released_ = false;
};

class VectorRelease final {
public:
    VectorRelease(std::uint32_t vector, ICombatConditionalActionsRuntime& runtime)
        : vector_(vector), runtime_(runtime) {}
    ~VectorRelease() { runtime_.FreeVector(vector_); }

private:
    std::uint32_t vector_;
    ICombatConditionalActionsRuntime& runtime_;
};

}  // namespace

std::int32_t CombatConditionalActions::Select(
    std::uint32_t action, std::uint32_t actor, std::uint32_t target,
    ICombatConditionalActionsRuntime& runtime) {
    const auto gate = CheckGate(action, actor, runtime);
    if (!gate.accepted) return gate.rejected_result;

    std::vector<std::int32_t> ids;
    IdRelease release(ids, runtime);
    for (std::uint32_t index = 0u; index < 7u; ++index) {
        const auto id = runtime.FloatToInt(
            ReadFloat(action + 28u + index * 4u, runtime));
        if (id != 0) ids.push_back(id);
    }
    const auto random = runtime.RandomNumber();
    if (ids.empty()) throw std::domain_error("native selection divides by zero");
    runtime.ApplySelected(target, ids[random % ids.size()]);
    return release.Release();
}

std::int32_t CombatConditionalActions::ResetMatching(
    std::uint32_t action, std::uint32_t actor, std::uint32_t target,
    ICombatConditionalActionsRuntime& runtime) {
    const auto gate = CheckGate(action, actor, runtime);
    if (!gate.accepted) return gate.rejected_result;

    for (std::uint32_t index = 0u; index < 7u; ++index) {
        const auto address = action + 28u + index * 4u;
        const auto id = runtime.FloatToInt(ReadFloat(address, runtime));
        if (id == 0) continue;
        const auto vector = runtime.EnumerateCandidates(
            target, static_cast<std::uint8_t>(address));
        VectorRelease release(vector, runtime);
        const auto begin = runtime.VectorBegin(vector);
        const auto end = runtime.VectorEnd(vector);
        for (auto at = begin; at != end; at += 4u) {
            const auto candidate = runtime.ReadWord(at);
            if (runtime.CandidateId(candidate) != id) continue;
            runtime.ResetCandidate(candidate, 7u, 0);
            runtime.RefreshCandidate(candidate);
        }
    }
    return 0;
}

}  // namespace pal4
