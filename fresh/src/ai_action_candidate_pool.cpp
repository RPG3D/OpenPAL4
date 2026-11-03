#include "pal4/ai_action_task.h"

namespace pal4 {
namespace {

struct IteratorOwner {
    IAiActionPoolRuntime& runtime;
    std::uint32_t iterator;
    ~IteratorOwner() {
        if (iterator != 0u) runtime.ReleaseIterator(iterator);
    }
};

void AppendEligible(std::vector<std::uint32_t>& candidates,
                    const std::uint32_t iterator,
                    const std::uint32_t mode,
                    const std::uint32_t excluded,
                    IAiActionPoolRuntime& runtime) {
    while (!runtime.IteratorEnd(iterator)) {
        const auto candidate = runtime.IteratorCurrent(iterator);
        if (runtime.IsEligible(candidate) &&
            (excluded == 0u || candidate != excluded) &&
            runtime.ActorCapability(candidate, mode)) {
            candidates.push_back(candidate);
        }
        runtime.IteratorNext(iterator);
    }
}

}  // namespace

std::vector<std::uint32_t> AiActionCandidatePool::Build(
    const std::uint32_t task, const std::uint32_t supplied_iterator,
    const std::uint32_t mode, IAiActionPoolRuntime& runtime) {
    std::vector<std::uint32_t> candidates;
    candidates.reserve(16u);
    const auto actor = runtime.ReadWord(task + 4u);
    if (runtime.ActorAnimationId(actor) == 98u ||
        runtime.ActorAnimationId(actor) == 99u) {
        {
            const IteratorOwner team{runtime,
                runtime.CreateActorIterator(actor, AiActorGroup::Team)};
            AppendEligible(candidates, team.iterator, mode, actor, runtime);
        }
        {
            const IteratorOwner enemies{runtime,
                runtime.CreateActorIterator(actor, AiActorGroup::Enemy)};
            AppendEligible(candidates, enemies.iterator, mode, 0u, runtime);
        }
    } else {
        runtime.IteratorReset(supplied_iterator);
        AppendEligible(candidates, supplied_iterator, mode, 0u,
                       runtime);
    }
    return candidates;
}

}  // namespace pal4
