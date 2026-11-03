#include "pal4/ai_action_task.h"

namespace pal4 {
namespace {

struct IteratorOwner {
    IAiActionTargetSelectionRuntime& runtime;
    std::uint32_t iterator;
    ~IteratorOwner() {
        if (iterator != 0u) runtime.ReleaseIterator(iterator);
    }
};

}  // namespace

std::uint32_t AiActionTargetSelection::Select(
    const std::uint32_t task, const std::uint32_t category,
    const std::uint32_t state, const std::uint32_t requested,
    const std::uint32_t mode,
    IAiActionTargetSelectionRuntime& runtime) {
    if (state == 5u) {
        const auto actor = runtime.ReadWord(task + 4u);
        std::vector<std::uint32_t> candidates;
        const IteratorOwner iterator{
            runtime, runtime.CreateActorIterator(actor,
                                                  AiActorGroup::Team)};
        while (!runtime.IteratorEnd(iterator.iterator)) {
            const auto member = runtime.IteratorCurrent(iterator.iterator);
            if (runtime.IsDead(member)) candidates.push_back(member);
            runtime.IteratorNext(iterator.iterator);
        }
        return runtime.SelectTarget(task, candidates, requested,
                                    mode, state);
    }

    if (category == 0u) return runtime.ReadWord(task + 4u);
    AiActorGroup group;
    switch (category) {
        case 1u:
        case 3u:
            group = AiActorGroup::Team;
            break;
        case 2u:
        case 4u:
            group = AiActorGroup::Enemy;
            break;
        default:
            return 0u;
    }
    std::vector<std::uint32_t> candidates;
    const auto actor = runtime.ReadWord(task + 4u);
    {
        const IteratorOwner iterator{
            runtime, runtime.CreateActorIterator(actor, group)};
        candidates = runtime.BuildPool(task, iterator.iterator, mode);
    }
    return runtime.SelectTarget(task, candidates, requested,
                                mode, state);
}

}  // namespace pal4
