#include "pal4/script_actor_event_processor.h"

#include <vector>

namespace pal4 {

std::uint32_t ScriptActorEventProcessor::SelectTarget(
    const std::uint32_t source, const std::uint32_t target,
    IScriptActorEventRuntime& runtime) {
    const auto target_state = runtime.ActorState(target);
    if (runtime.ActorState(source) == target_state ||
        target_state != 1 || runtime.SourceBlocksEvent(source))
        return 0u;

    const auto iterator = runtime.OpenTargetIterator(target);
    struct IteratorGuard {
        IScriptActorEventRuntime& runtime;
        std::uint32_t iterator;
        ~IteratorGuard() {
            if (iterator != 0u) runtime.DeleteIterator(iterator, 1u);
        }
    } guard{runtime, iterator};

    std::vector<std::uint32_t> candidates;
    while (!runtime.IteratorAtEnd(iterator)) {
        const auto candidate = runtime.IteratorCurrent(iterator);
        if (runtime.CandidateStateNotThree(candidate) &&
            candidate != target &&
            runtime.CandidateCanAffect(candidate, target))
            candidates.push_back(candidate);
        runtime.IteratorNext(iterator);
    }
    if (candidates.empty()) return 0u;
    return candidates[runtime.GenerateRandomNumber() % candidates.size()];
}

}  // namespace pal4
