#include "pal4/combat_action_followups.h"

#include <bit>

namespace pal4 {
namespace {

class VectorRelease final {
public:
    VectorRelease(std::uint32_t vector, ICombatActionFollowupsRuntime& runtime)
        : vector_(vector), runtime_(runtime) {}
    ~VectorRelease() {
        if (!released_) static_cast<void>(runtime_.FreeVector(vector_));
    }
    std::int32_t Release() {
        released_ = true;
        return runtime_.FreeVector(vector_);
    }

private:
    std::uint32_t vector_;
    ICombatActionFollowupsRuntime& runtime_;
    bool released_ = false;
};

float ReadFloat(std::uint32_t address, ICombatActionFollowupsRuntime& runtime) {
    return std::bit_cast<float>(runtime.ReadWord(address));
}

}  // namespace

std::int32_t CombatActionFollowups::ResetOtherTargets(
    std::uint32_t actor, ICombatActionFollowupsRuntime& runtime) {
    const auto vector = runtime.EnumerateTargets(
        actor, static_cast<std::uint8_t>(actor));
    VectorRelease release(vector, runtime);
    const auto begin = runtime.VectorBegin(vector);
    const auto end = runtime.VectorEnd(vector);
    for (auto at = begin; at != end; at += 4u) {
        const auto candidate = runtime.ReadWord(at);
        if (candidate == actor) continue;
        runtime.ResetSlot(candidate, 7u, 0);
        runtime.RefreshActor(candidate);
    }
    return release.Release();
}

std::int32_t CombatActionFollowups::Transfer(
    std::uint32_t action, std::uint32_t target,
    std::uint32_t source, ICombatActionFollowupsRuntime& runtime) {
    const auto value = runtime.FloatToInt(ReadFloat(action + 20u, runtime));
    const auto negative = std::bit_cast<std::int32_t>(
        0u - std::bit_cast<std::uint32_t>(value));
    static_cast<void>(runtime.TransferValue(source, negative));
    return runtime.TransferValue(target, value);
}

std::uint8_t CombatActionFollowups::CleanupConditions(
    std::uint32_t action, std::uint32_t actor,
    ICombatActionFollowupsRuntime& runtime) {
    bool any = false;
    for (std::uint32_t index = 0u; index < 9u; ++index) {
        const auto id = runtime.FloatToInt(
            ReadFloat(action + 20u + index * 4u, runtime));
        if (id == 0) continue;
        runtime.CleanupCondition(actor, id, 0u);
        any = true;
    }
    return any ? static_cast<std::uint8_t>(runtime.NotifyCleanup(actor)) : 0u;
}

}  // namespace pal4
