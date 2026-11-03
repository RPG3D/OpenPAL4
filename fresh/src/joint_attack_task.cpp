#include "pal4/joint_attack_task.h"

#include <vector>

namespace pal4 {

std::uint8_t JointAttackTask::Process(
    const std::uint32_t task, IJointAttackRuntime& runtime) {
    ProcessInternal(task, runtime);
    return 1u;
}

bool JointAttackTask::IsAttackable(
    const std::uint32_t actor, IJointAttackRuntime& runtime) {
    if (!runtime.NotStateThree(actor)) return false;
    const auto mode = runtime.ActorMode(actor);
    return mode == 0u || (mode > 1u && mode <= 3u);
}

void JointAttackTask::ProcessInternal(
    const std::uint32_t task, IJointAttackRuntime& runtime) {
    const auto target = runtime.ReadWord(task + 64u);
    if (runtime.StateThree(target)) return;
    if (runtime.Value7(target) <= 0) return;

    const auto attacker = runtime.ReadWord(task + 60u);
    const auto attacker_faction = runtime.ReadWord(attacker + 804u);
    const auto target_faction = runtime.ReadWord(target + 804u);
    if (attacker_faction == target_faction || attacker_faction != 1u) return;

    const auto chance_threshold = runtime.GameDBFloat(15u);
    const auto property_threshold = runtime.GameDBFloat(16u);
    const auto roll = static_cast<double>(runtime.RandomNumber() % 1000u) *
                      0.001;
    if (roll > static_cast<double>(chance_threshold)) return;

    const auto iterator = runtime.CreateIterator(attacker);
    struct IteratorGuard final {
        IJointAttackRuntime& runtime;
        std::uint32_t iterator;
        ~IteratorGuard() {
            if (iterator) runtime.ReleaseIterator(iterator);
        }
    } guard{runtime, iterator};
    if (!iterator) return;

    std::vector<std::uint32_t> candidates;
    while (!runtime.IteratorAtEnd(iterator)) {
        const auto candidate = runtime.IteratorCurrent(iterator);
        if (candidate != attacker && IsAttackable(candidate, runtime)) {
            const auto property = static_cast<double>(
                runtime.ActorProperty(candidate + 716u)) * 0.015625;
            if (property > static_cast<double>(property_threshold))
                candidates.push_back(candidate);
        }
        runtime.IteratorNext(iterator);
    }
    if (candidates.empty()) return;
    const auto index = runtime.RandomNumber() % candidates.size();
    runtime.EmitRespond("joint_attack_respond", candidates[index], target, 1u);
}

}  // namespace pal4
