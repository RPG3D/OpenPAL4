#include "pal4/end_attack_task.h"

#include "pal4/check_protect_task.h"
#include "pal4/start_attack_task.h"

namespace pal4 {

EndAttackBaseFactorySlot* EndAttackBaseFactory::Initialize(
    EndAttackBaseFactorySlot* const slot) {
    slot->vtable = kVtable;
    return slot;
}

std::uint32_t EndAttackTask::DestroyBase(
    const std::uint32_t task, IScriptTaskBaseRuntime& runtime) {
    return ScriptTaskBase::Destroy(task, runtime);
}

std::uint32_t EndAttackTask::Start(
    const std::uint32_t task, IEndAttackStartRuntime& runtime) {
    constexpr std::array<std::uint32_t, 3> timing{1u, 0u, 0u};
    const auto base_text = runtime.GlobalText(0x8f966cu);
    EndAttackBaseFactorySlot base_factory;
    (void)EndAttackBaseFactory::Initialize(&base_factory);
    const auto base_child = runtime.CreateChildTask(base_factory.vtable);
    if (base_child == 0u) return 0u;
    runtime.AddTaskAndInitialize(task + 4u, base_child, base_text, timing);
    const auto source = runtime.ReadWord(task + 44u);
    const auto target = runtime.ReadWord(task + 48u);
    runtime.WriteWord(base_child + 64u, target);
    runtime.WriteWord(base_child + 60u, source);

    if (runtime.ReadByte(task + 53u) != 0u) {
        const auto position = runtime.ActorTargetPosition(target);
        const auto position_text = runtime.GlobalText(0x8f9798u);
        CheckProtectSubtaskFactorySlot position_factory;
        (void)CheckProtectSubtaskFactory::InitializePosition(
            &position_factory);
        const auto position_child = runtime.CreateChildTask(
            position_factory.vtable);
        if (position_child == 0u) return 0u;
        runtime.AddTaskAndInitialize(task + 4u, position_child,
                                     position_text, timing);
        runtime.WriteWord(position_child + 72u, target);
        for (std::uint32_t i = 0u; i < 3u; ++i)
            runtime.WriteFloat(position_child + 60u + 4u * i,
                               position[i]);
    }

    const auto completion = runtime.SourceEndsSequence(source);
    if (completion != 0u) return completion;
    const auto source_state = runtime.ReadWord(source + 804u);
    const auto target_state = runtime.ReadWord(target + 804u);
    if (source_state == target_state) return source_state;

    StartAttackApproachFactorySlot run_factory;
    const auto first_text = runtime.GlobalText(0x8f9784u);
    (void)StartAttackApproachFactory::InitializePrimary(&run_factory);
    const auto first = runtime.CreateChildTask(run_factory.vtable);
    if (first == 0u) return 0u;
    runtime.AddTaskAndInitialize(task + 4u, first,
                                 first_text, timing);
    const auto first_position = runtime.ActorTargetPosition(source);
    (void)runtime.SetApproachTaskTarget(first, source, first_position);

    const auto second_text = runtime.GlobalText(0x8f97acu);
    (void)StartAttackApproachFactory::InitializeSecondary(&run_factory);
    const auto second = runtime.CreateChildTask(run_factory.vtable);
    if (second == 0u) return 0u;
    runtime.AddTaskAndInitialize(task + 4u, second,
                                 second_text, timing);
    const auto second_position = runtime.ActorTargetPosition(source);
    return runtime.SetApproachTaskTarget(second, source, second_position);
}

}  // namespace pal4
