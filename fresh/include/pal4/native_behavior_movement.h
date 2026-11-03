#pragma once
#include "pal4/behavior_path_endpoint_helper.h"
#include <functional>
#include <span>
#include <vector>

namespace pal4 {
class NativeBehaviorGridAssociationRuntime;
struct NativeBehaviorMovementStats {
    std::uint32_t frames{},random_draws{},sphere_queries{},neighbor_visits{};
    std::uint32_t candidate_tests{},candidate_hits{},proximity{},predicted{};
    std::uint32_t drift_updates{},path_corrections{},direction_flips{};
};
struct NativeBehaviorMovementResult {
    bool active{};
    BehaviorVector3 requested{};
};
// One native collision/query implementation for endpoint0/1 and random3 helpers.
// Static obstacles are actual secondary-list sphere records, not helper payloads.
// Construction, configuration and all updates must use the same random stream.
class NativeBehaviorMovement {
public:
    using Random=std::function<std::uint32_t()>;
    NativeBehaviorMovement(NativeBehaviorGridAssociationRuntime&,Random);
    NativeBehaviorMovementResult UpdateRandom(std::uint32_t helper,
        std::span<const std::uint32_t> obstacles,float elapsed);
    NativeBehaviorMovementResult UpdatePath(std::uint32_t helper,
        std::span<const std::uint32_t> obstacles,float elapsed);
    const NativeBehaviorMovementStats& Stats() const noexcept { return stats_; }
private:
    class Runtime;
    NativeBehaviorGridAssociationRuntime& associations_;
    Random random_;
    NativeBehaviorMovementStats stats_{};
    std::vector<std::uint32_t> neighbors_;
};
}
