#include "pal4/actor_manager_target_collector.h"

namespace pal4 {
namespace {

bool Matches(std::uint32_t mode, std::uint32_t target,
             IActorManagerTargetCollectorRuntime& runtime) {
    const auto type = runtime.TargetType(target);
    switch (mode) {
        case 0u:
        case 2u:
            return type == 1u;
        case 1u:
            return type == 0u || runtime.TargetType(target) == 2u;
        case 3u:
        case 5u:
            return type == 4u;
        case 4u:
            return type == 3u || runtime.TargetType(target) == 5u;
        default:
            return false;
    }
}

}  // namespace

std::uint32_t ActorManagerTargetCollector::Collect(
    std::uint32_t actor, std::uint32_t collection,
    IActorManagerTargetCollectorRuntime& runtime) {
    const auto mode = runtime.SelectionMode(actor);
    const auto iterator = runtime.CreateIterator(actor);
    if (mode <= 5u) {
        while (!runtime.AtEnd(iterator)) {
            const auto target = runtime.Current(iterator);
            if (Matches(mode, target, runtime)) {
                if (mode == 0u) runtime.InsertDirect(collection, target);
                else runtime.InsertEvent(collection, target);
            }
            runtime.Advance(iterator);
        }
    }
    return iterator == 0u ? 0u : runtime.DestroyIterator(iterator);
}

std::uint32_t ActorManagerTargetCollector::CollectAll(
    std::uint32_t actor, std::uint32_t collection,
    IActorManagerTargetCollectorRuntime& runtime) {
    const auto iterator = runtime.CreateIterator(actor);
    while (!runtime.AtEnd(iterator)) {
        runtime.InsertEvent(collection, runtime.Current(iterator));
        runtime.Advance(iterator);
    }
    return iterator == 0u ? 0u : runtime.DestroyIterator(iterator);
}

}  // namespace pal4
