#include "pal4/end_attack_task.h"

#include "pal4/start_attack_task.h"

#include <bit>

namespace pal4 {

std::uint32_t EndAttackTask::CreateNext(
    const std::uint32_t task, IEndAttackTransitionRuntime& runtime) {
    const auto source = runtime.ReadWord(task + 44u);
    runtime.NotifySource(source);
    const auto combat = runtime.CombatSystem();
    const auto ready = runtime.AllCombatObjectsReady(combat);
    runtime.DispatchEvent(runtime.ReadWord(0x8b8b58u), 0u, 0u, 1u);
    if (!ready || runtime.ReadByte(task + 52u) == 0u) return 0u;

    const auto target = runtime.ReadWord(task + 48u);
    if (!runtime.IsActorReady(target)) return 0u;
    const auto chance = runtime.ActorChance(target, 4u);
    const auto random = runtime.RandomNumber() % 1000u;
    if (!(static_cast<double>(chance) >
          static_cast<double>(random) * static_cast<double>(0.001f)))
        return 0u;
    if (runtime.SourceStatus(source) == 4u ||
        runtime.TargetStatus(target) == 1u)
        return 0u;
    runtime.TargetSetSource(target, source);

    const auto next = runtime.Allocate(StartAttackTask::kSize);
    if (next == 0u) return 0u;
    struct Guard {
        IEndAttackTransitionRuntime& runtime;
        std::uint32_t task;
        bool container_initialized = false;
        bool name_initialized = false;
        ~Guard() {
            if (task == 0u) return;
            if (name_initialized) runtime.CleanupName(task + 20u);
            if (container_initialized) runtime.DestroyContainer(task + 4u);
            runtime.Free(task);
        }
    } guard{runtime, next};

    const auto attack_target = runtime.ReadWord(task + 48u);
    const auto attack_source = runtime.ReadWord(task + 44u);
    runtime.InitializeContainer(next + 4u);
    guard.container_initialized = true;
    runtime.WriteByte(next + 20u,
                      static_cast<std::uint8_t>(
                          std::bit_cast<std::uint32_t>(chance) >> 24u));
    runtime.WriteWord(next + 24u, 0u);
    runtime.WriteWord(next + 28u, 0u);
    runtime.WriteWord(next + 32u, 0u);
    guard.name_initialized = true;
    runtime.AssignName(next + 20u, "StartAttack");
    runtime.WriteWord(next + 48u, attack_source);
    runtime.WriteWord(next + 36u, 0u);
    runtime.WriteByte(next + 40u, 1u);
    runtime.WriteWord(next + 44u, attack_target);
    runtime.WriteByte(next + 52u, 0u);
    runtime.WriteByte(next + 53u, 0u);
    runtime.WriteByte(next + 54u, 0u);
    runtime.WriteWord(next, StartAttackTask::kVtable);
    guard.task = 0u;
    return next;
}

}  // namespace pal4
