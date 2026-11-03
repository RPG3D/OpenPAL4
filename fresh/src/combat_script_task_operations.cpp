#include "pal4/combat_script_task_operations.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace pal4 {

std::uint8_t CombatScriptTaskOperations::FireEvent(
    const std::uint32_t task, ICombatScriptTaskRuntime& runtime) {
    const auto actor = runtime.ReadWord(task + 60u);
    if (actor) {
        const auto callback = runtime.ReadWord(task + 64u);
        if (callback)
            runtime.FireEvent(callback, actor, runtime.ReadWord(task + 68u));
    }
    return 1u;
}

std::uint8_t CombatScriptTaskOperations::SetProtectTarget(
    const std::uint32_t task, ICombatProtectTargetRuntime& runtime) {
    const auto actor = runtime.ReadWord(task + 64u);
    const auto argument = runtime.ReadWord(task + 68u);
    const auto target = runtime.ReadWord(task + 60u);
    runtime.SetProtectTarget(actor, target, argument);
    return 1u;
}

std::uint32_t NormalAttackTask::Construct(
    const std::uint32_t task, ICombatScriptTaskRuntime& runtime) {
    runtime.ConstructBase(task);
    runtime.WriteWord(task, kVtable);
    runtime.WriteWord(task + 64u, 0u);
    runtime.WriteWord(task + 60u, 0u);
    return task;
}

std::uint32_t NormalAttackTask::Create(
    INormalAttackCreationRuntime& runtime) {
    const auto task = runtime.Allocate(kSize);
    if (task == 0u) return 0u;
    try {
        return Construct(task, runtime);
    } catch (...) {
        runtime.Free(task);
        throw;
    }
}

std::uint8_t NormalAttackTask::Process(
    const std::uint32_t task, INormalAttackProcessRuntime& runtime) {
    ProcessInternal(task, runtime);
    return 1u;
}

void NormalAttackTask::ProcessInternal(
    const std::uint32_t task, INormalAttackProcessRuntime& runtime) {
    const auto actor = runtime.ReadWord(task + 60u);
    if (!actor) return;
    const auto target = runtime.ReadWord(task + 64u);
    if (!target) return;

    runtime.PlayAttackSound(actor, 1u);
    if (runtime.ActorMode(actor) != 1u) {
        runtime.WriteScale(task + 68u, 1.0f);
        runtime.HandleActor(task, target);
        return;
    }

    runtime.WriteScale(task + 68u, runtime.DamageScale(26u));
    const auto iterator = runtime.TargetIterator(target);
    struct IteratorGuard final {
        INormalAttackProcessRuntime& runtime;
        std::uint32_t iterator;
        ~IteratorGuard() {
            if (iterator) runtime.ReleaseIterator(iterator);
        }
    } guard{runtime, iterator};
    if (!iterator) return;
    while (!runtime.IteratorAtEnd(iterator)) {
        const auto candidate = runtime.IteratorCurrent(iterator);
        if (runtime.ActorEligible(candidate))
            runtime.HandleActor(task, candidate);
        runtime.IteratorNext(iterator);
    }
}

std::uint8_t NormalAttackTask::HandleActor(
    const std::uint32_t task, const std::uint32_t actor,
    INormalAttackActorRuntime& runtime) {
    if (!runtime.HasProperty8(actor))
        return runtime.NoPropertyTarget(actor, runtime.ReadWord(task + 60u));
    if (runtime.ReadByte(task + 72u))
        return runtime.DispatchActor(task, actor,
                                     NormalAttackActorBranch::Item);

    const auto attacker = runtime.ReadWord(task + 60u);
    constexpr std::uint32_t kCombatDataOffset = 340u;
    const auto attacker_data = attacker + kCombatDataOffset;
    const auto target_data = actor + kCombatDataOffset;
    if (runtime.CheckHit(attacker_data, target_data))
        return runtime.DispatchActor(task, actor,
                                     NormalAttackActorBranch::Magic);

    bool skip_float_property = false;
    if (runtime.ReadByte(task + 73u) ||
        runtime.TargetMode(runtime.ReadWord(task + 64u)) == 1u) {
        skip_float_property =
            runtime.TargetMode(runtime.ReadWord(task + 64u)) == 4u;
    }
    if (!skip_float_property &&
        runtime.CheckFloatProperty(attacker_data, target_data))
        return runtime.DispatchActor(task, actor,
                                     NormalAttackActorBranch::Item);

    const auto dodged = runtime.CheckDodge(attacker_data, target_data, 0.0f);
    if (runtime.ReadByte(task + 73u) || dodged ||
        runtime.TargetMode(runtime.ReadWord(task + 64u)) == 4u)
        return runtime.DispatchActor(task, actor,
                                     NormalAttackActorBranch::Attack);
    return runtime.DispatchActor(task, actor,
                                 NormalAttackActorBranch::Support);
}

std::uint32_t NormalAttackTask::ProcessCommand(
    const std::uint32_t task, const std::string_view message_name,
    INormalAttackCommandRuntime& runtime) {
    std::string command(message_name);
    std::transform(command.begin(), command.end(), command.begin(),
                   [](const unsigned char value) {
                       return static_cast<char>(std::tolower(value));
                   });
    if (command.size() > 2u && command.compare(0, 3u, "atk") == 0)
        runtime.ProcessAttack(task + 0xd8u);
    else
        runtime.ProcessAnimationCommand(task, message_name);
    return runtime.CleanupCommand(command);
}

std::uint8_t NormalAttackTask::SetTarget(
    const std::uint32_t task, INormalAttackCommandRuntime& runtime) {
    const auto actor = runtime.ReadWord(task + 276u);
    runtime.NotifyTargetState(actor, 14u, actor);
    return 1u;
}

}  // namespace pal4
