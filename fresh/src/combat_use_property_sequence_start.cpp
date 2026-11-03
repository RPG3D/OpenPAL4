#include "pal4/combat_use_property_sequence_task.h"

#include "pal4/start_attack_task.h"
#include "pal4/use_property_task.h"

namespace pal4 {
namespace {

constexpr std::array<std::uint32_t, 3> kTiming{1u, 0u, 0u};
constexpr std::uint32_t kEmptyText = 0x841964u;

std::uint32_t TextOrEmpty(IUsePropertySequenceStartRuntime& runtime,
                          const std::uint32_t global) {
    const auto text = runtime.ReadWord(global);
    return text == 0u ? kEmptyText : text;
}

std::uint32_t CreateChild(const std::uint32_t task,
                          const std::uint32_t factory,
                          const std::uint32_t text_global,
                          IUsePropertySequenceStartRuntime& runtime) {
    const auto text = TextOrEmpty(runtime, text_global);
    const auto child = runtime.CreateChildTask(factory);
    if (child != 0u)
        runtime.AddTaskAndInitialize(task + 4u, child, text, kTiming);
    return child;
}

}  // namespace

UsePropertyChildFactorySlot* UsePropertyChildFactories::InitializeCast(
    UsePropertyChildFactorySlot* const slot) {
    slot->vtable = kCastVtable;
    return slot;
}

UsePropertyChildFactorySlot* UsePropertyChildFactories::InitializeThrow(
    UsePropertyChildFactorySlot* const slot) {
    slot->vtable = kThrowVtable;
    return slot;
}

UsePropertyChildFactorySlot* UsePropertyChildFactories::InitializeDirect(
    UsePropertyChildFactorySlot* const slot) {
    slot->vtable = kDirectVtable;
    return slot;
}

std::uint32_t UsePropertySequenceTask::Start(
    const std::uint32_t task, IUsePropertySequenceStartRuntime& runtime) {
    const auto target = runtime.ReadWord(task + 48u);
    auto position = runtime.ActorChildPosition(target);
    const auto property = runtime.ReadWord(task + 52u);
    if (runtime.PropertyMovementMode(property) == 4u)
        position = runtime.ActorTargetPosition(runtime.ReadWord(task + 48u));

    runtime.NotifySource(runtime.ReadWord(task + 44u));
    const auto mode_property = runtime.ReadWord(task + 52u);
    const auto first_mode = runtime.PropertyMovementMode(mode_property);
    if (first_mode == 2u ||
        runtime.PropertyMovementMode(runtime.ReadWord(task + 52u)) == 4u) {
        if (runtime.ReadWord(task + 44u) != runtime.ReadWord(task + 48u)) {
            const auto run = CreateChild(
                task, StartAttackApproachFactory::kPrimaryVtable,
                0x8f9784u, runtime);
            if (run == 0u) return 0u;
            runtime.SetApproachTaskTarget(run, runtime.ReadWord(task + 44u),
                                     position);
        }

        if (runtime.ReadWord(runtime.ReadWord(task + 52u) + 156u) != 0u) {
            UsePropertyChildFactorySlot factory;
            (void)UsePropertyChildFactories::InitializeCast(&factory);
            const auto child = CreateChild(task, factory.vtable,
                                           0x8f97e8u, runtime);
            if (child == 0u) return 0u;
            const auto current_property = runtime.ReadWord(task + 52u);
            const auto current_target = runtime.ReadWord(task + 48u);
            const auto current_source = runtime.ReadWord(task + 44u);
            runtime.WriteWord(child + 60u, current_source);
            (void)runtime.AssignChildText(child + 64u, 0x8fa2e8u);
            const auto result = UsePropertyTask::SetActorsAndProperty(
                child + 232u, current_source, current_target,
                current_property, runtime);
            runtime.WriteByte(child + 326u, 1u);
            return result;
        }

        UsePropertyChildFactorySlot factory;
        (void)UsePropertyChildFactories::InitializeThrow(&factory);
        const auto child = CreateChild(task, factory.vtable,
                                       0x8f97fcu, runtime);
        if (child == 0u) return 0u;
        const auto current_property = runtime.ReadWord(task + 52u);
        const auto current_target = runtime.ReadWord(task + 48u);
        const auto current_source = runtime.ReadWord(task + 44u);
        runtime.WriteWord(child + 60u, current_source);
        (void)runtime.AssignChildText(child + 64u, 0x8fa2e8u);
        runtime.WriteWord(child + 388u, current_source);
        runtime.WriteWord(child + 392u, current_target);
        runtime.WriteWord(child + 396u, current_property);
        const auto result = runtime.SetThrowPropertyTargets(
            child + 240u, current_source, current_target, current_property);
        runtime.WriteByte(child + 410u, 1u);
        return result;
    }

    UsePropertyChildFactorySlot factory;
    (void)UsePropertyChildFactories::InitializeDirect(&factory);
    const auto child = CreateChild(task, factory.vtable, 0x8f9824u,
                                   runtime);
    if (child == 0u) return 0u;
    const auto current_property = runtime.ReadWord(task + 52u);
    const auto current_target = runtime.ReadWord(task + 48u);
    const auto current_source = runtime.ReadWord(task + 44u);
    runtime.WriteWord(child + 60u, current_source);
    const auto result = runtime.AssignChildText(child + 64u, 0x8fa2f8u);
    runtime.WriteWord(child + 300u, current_source);
    runtime.WriteWord(child + 304u, current_target);
    runtime.WriteWord(child + 308u, current_property);
    runtime.WriteByte(child + 320u, 1u);
    return result;
}

std::uint32_t UsePropertySequenceTask::CreateNext(
    const std::uint32_t task, IUsePropertyTransitionRuntime& runtime) {
    const auto source = runtime.ReadWord(task + 44u);
    if (!runtime.SourceStateThree(source))
        return CombatEndActionFactory::Create(
            runtime.ReadWord(task + 44u), runtime);
    std::uint32_t owned{};
    if (runtime.TryCreateDeadWithEndAction(source,runtime.ReadWord(task + 48u),owned)) return owned;
    const auto dead = CombatDeadActionFactory::Create(
        runtime.ReadWord(task + 44u),
        runtime.ReadWord(task + 48u), runtime);
    if (dead == 0u) return 0u;
    runtime.WriteWord(dead + 36u, CombatEndActionFactory::Create(
        runtime.ReadWord(task + 44u), runtime));
    return dead;
}

}  // namespace pal4
