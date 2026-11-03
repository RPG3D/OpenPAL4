#include "pal4/quick_attack_task.h"

namespace pal4 {
namespace {

std::uint32_t BuildNextTask(const std::uint32_t parent,
                            const bool repeat,
                            IQuickAttackTransitionRuntime& runtime) {
    const auto task = runtime.Allocate(QuickAttackTask::kSize);
    if (task == 0u) return 0u;
    struct Guard {
        IQuickAttackTransitionRuntime& runtime;
        std::uint32_t task;
        bool container_initialized = false;
        bool name_initialized = false;
        ~Guard() {
            if (task == 0u) return;
            if (name_initialized) runtime.CleanupName(task + 20u);
            if (container_initialized) runtime.DestroyContainer(task + 4u);
            runtime.Free(task);
        }
    } guard{runtime, task};

    const std::uint8_t flag_53 = repeat ? std::uint8_t{0}
                                         : runtime.ReadByte(parent + 53u);
    const auto target = runtime.ReadWord(parent + 48u);
    const auto flag_52 = runtime.ReadByte(parent + 52u);
    const auto source = runtime.ReadWord(parent + 44u);
    runtime.InitializeContainer(task + 4u);
    guard.container_initialized = true;

    const auto name = repeat ? std::string_view("QuickAttack")
                             : std::string_view("cbQuickEnd");
    // The repeat branch reads an unwritten stack byte for this field.
    runtime.WriteByte(task + 20u,
                      repeat ? std::uint8_t{0} : flag_53);
    if (repeat) {
        runtime.SetNameErrorFlag(task + 20u, 0u);
    } else {
        runtime.WriteWord(task + 24u, 0u);
        runtime.WriteWord(task + 28u, 0u);
        runtime.WriteWord(task + 32u, 0u);
    }
    guard.name_initialized = true;
    if (runtime.AllocateNameBuffer(task + 20u,
                                   static_cast<std::uint32_t>(name.size()),
                                   1u)) {
        runtime.CopyName(task + 20u, name);
        runtime.WriteWord(task + 28u,
                          static_cast<std::uint32_t>(name.size()));
    }
    runtime.WriteWord(task + 36u, 0u);
    runtime.WriteByte(task + 40u, 1u);
    runtime.WriteWord(task + 44u, source);
    runtime.WriteWord(task + 48u, target);
    runtime.WriteByte(task + 52u, flag_52);
    runtime.WriteByte(task + 53u,
                      repeat ? std::uint8_t{0} : flag_53);
    runtime.WriteWord(task, repeat ? QuickAttackTask::kVtable
                                   : QuickAttackTask::kEndVtable);
    guard.task = 0u;
    return task;
}

}  // namespace

std::uint32_t QuickAttackTask::CreateNext(
    const std::uint32_t task,
    IQuickAttackTransitionRuntime& runtime) {
    if (!runtime.CombatActive()) return 0u;
    const auto source = runtime.ReadWord(task + 44u);
    if (runtime.SourceStateThree(source)) {
        const auto dead = runtime.ProcessDead(
            source, runtime.ReadWord(task + 48u));
        const auto end_action = runtime.EndAction(
            runtime.ReadWord(task + 44u));
        if (dead == 0u) return 0u;
        runtime.WriteWord(dead + 36u, end_action);
        return dead;
    }
    const auto count = runtime.ActorCounter(source, 3u);
    if (count > 0)
        runtime.SetActorCounter(source, 3u, count - 1);
    return BuildNextTask(task, count > 0, runtime);
}

}  // namespace pal4
