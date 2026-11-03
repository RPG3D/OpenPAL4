#include "pal4/combat_stunt_sequence_task.h"

#include "pal4/script_task_base.h"
#include "pal4/start_attack_task.h"
#include "pal4/check_protect_task.h"

#include <bit>

namespace pal4 {

std::uint32_t EndStuntSequenceTask::DestroyBase(
    const std::uint32_t task, IScriptTaskBaseRuntime& runtime) {
    return ScriptTaskBase::Destroy(task, runtime);
}

std::uint32_t EndStuntSequenceTask::Destroy(
    const std::uint32_t task, const std::uint8_t flags,
    IScriptTaskBaseRuntime& runtime) {
    (void)DestroyBase(task, runtime);
    if ((flags & 1u) != 0u) runtime.Free(task);
    return task;
}

namespace {

constexpr std::array<std::uint32_t, 3> kTiming{1u, 0u, 0u};
constexpr std::uint32_t kEmptyText = 0x841964u;

std::uint32_t TextOrEmpty(IStuntSequenceRuntime& runtime,
                          const std::uint32_t global) {
    const auto text = runtime.ReadWord(global);
    return text == 0u ? kEmptyText : text;
}

bool AddRunTask(const std::uint32_t task,
                const std::uint32_t factory_vtable,
                const std::uint32_t text_global,
                const std::array<float, 3>& position,
                IStuntSequenceRuntime& runtime) {
    const auto text = TextOrEmpty(runtime, text_global);
    const auto child = runtime.CreateChildTask(factory_vtable);
    if (child == 0u) return false;
    runtime.AddTaskAndInitialize(task + 4u, child, text, kTiming);
    runtime.SetApproachTaskTarget(child, runtime.ReadWord(task + 44u), position);
    return true;
}

}  // namespace

StuntChildFactorySlot* StuntChildFactory::Initialize(
    StuntChildFactorySlot* const slot) {
    slot->vtable = kVtable;
    return slot;
}

QuickStuntChildFactorySlot* QuickStuntChildFactory::Initialize(
    QuickStuntChildFactorySlot* const slot) {
    slot->vtable = kVtable;
    return slot;
}

std::uint32_t QuickStuntSequenceTask::Start(
    const std::uint32_t task, IQuickStuntStartRuntime& runtime) {
    constexpr std::array<std::uint32_t, 3> timing{1u, 0u, 0u};
    runtime.NotifySource(runtime.ReadWord(task + 44u));
    const auto configured_text = runtime.ReadWord(0x8f97d4u);
    const auto text = configured_text == 0u ? 0x841964u
                                             : configured_text;
    QuickStuntChildFactorySlot factory;
    (void)QuickStuntChildFactory::Initialize(&factory);
    const auto child = runtime.CreateChildTask(factory.vtable);
    if (child == 0u) return 0u;
    runtime.AddTaskAndInitialize(task + 4u, child, text, timing);
    const auto stunt = runtime.ReadWord(task + 52u);
    const auto source = runtime.ReadWord(task + 44u);
    runtime.WriteWord(child + 64u, runtime.ReadWord(task + 48u));
    runtime.WriteWord(child + 60u, source);
    runtime.WriteWord(child + 68u, stunt);
    return stunt;
}

std::uint8_t StuntSequenceTask::Start(
    const std::uint32_t task, IStuntSequenceRuntime& runtime) {
    const auto initial_source = runtime.ReadWord(task + 44u);
    if (runtime.ReadWord(initial_source + 808u) == 0u)
        return static_cast<std::uint8_t>(initial_source);

    const auto target = runtime.ReadWord(task + 48u);
    auto target_position = runtime.ActorChildPosition(target);
    const auto source_position =
        runtime.ActorChildPosition(runtime.ReadWord(task + 44u));
    std::array<float, 3> direction{};
    for (std::size_t i = 0u; i < direction.size(); ++i)
        direction[i] = source_position[i] - target_position[i];
    runtime.NormalizeVector(direction);
    const auto clamped_size = runtime.ActorClampedChildSize(
        runtime.ReadWord(task + 48u));
    for (auto& component : direction) component *= clamped_size;

    const auto target_for_side = runtime.ReadWord(task + 48u);
    auto source = runtime.ReadWord(task + 44u);
    const auto different_sides =
        runtime.ReadWord(target_for_side + 804u) !=
        runtime.ReadWord(source + 804u);
    if (different_sides)
        for (std::size_t i = 0u; i < target_position.size(); ++i)
            target_position[i] += direction[i];
    runtime.NotifySource(runtime.ReadWord(task + 44u));

    if (runtime.ReadWord(task + 44u) !=
        runtime.ReadWord(task + 48u)) {
        const auto stunt = runtime.ReadWord(task + 52u);
        const auto mode = runtime.StuntMovementMode(stunt);
        const auto text = TextOrEmpty(runtime, 0x8f9784u);
        StartAttackApproachFactorySlot run_factory;
        (void)StartAttackApproachFactory::InitializePrimary(&run_factory);
        const auto run = runtime.CreateChildTask(run_factory.vtable);
        if (run == 0u) return 0u;
        runtime.AddTaskAndInitialize(task + 4u, run, text, kTiming);
        const auto position = mode == 3u || mode == 4u
            ? runtime.ActorTargetPosition(runtime.ReadWord(task + 48u))
            : target_position;
        runtime.SetApproachTaskTarget(run, runtime.ReadWord(task + 44u),
                                 position);
    }

    const auto stunt = runtime.ReadWord(task + 52u);
    const auto effect_mode = runtime.ReadWord(stunt + 132u);
    bool sides_differ = false;
    if (effect_mode == 1u || effect_mode == 2u) {
        source = runtime.ReadWord(task + 44u);
        const auto current_target = runtime.ReadWord(task + 48u);
        sides_differ = runtime.ReadWord(source + 804u) !=
                       runtime.ReadWord(current_target + 804u);
    }
    if (effect_mode == 1u && sides_differ) {
        if (!AddRunTask(task, StartAttackApproachFactory::kSecondaryVtable,
                        0x8f97acu, target_position, runtime))
            return 0u;
    } else if (effect_mode == 2u && sides_differ) {
        if (!AddRunTask(task, StartAttackApproachFactory::kPrimaryVtable,
                        0x8f9784u, target_position, runtime))
            return 0u;
        const auto position_text = TextOrEmpty(runtime, 0x8f9798u);
        CheckProtectSubtaskFactorySlot position_factory;
        (void)CheckProtectSubtaskFactory::InitializePosition(
            &position_factory);
        const auto position_child = runtime.CreateChildTask(
            position_factory.vtable);
        if (position_child == 0u) return 0u;
        runtime.AddTaskAndInitialize(task + 4u, position_child,
                                     position_text, kTiming);
        runtime.WriteWord(position_child + 72u,
                          runtime.ReadWord(task + 44u));
        for (std::size_t i = 0u; i < target_position.size(); ++i)
            runtime.WriteWord(position_child + 60u +
                                  static_cast<std::uint32_t>(i) * 4u,
                              std::bit_cast<std::uint32_t>(target_position[i]));
    }

    const auto text = TextOrEmpty(runtime, 0x8f97c0u);
    StuntChildFactorySlot factory;
    (void)StuntChildFactory::Initialize(&factory);
    const auto child = runtime.CreateChildTask(factory.vtable);
    if (child == 0u) return 0u;
    runtime.AddTaskAndInitialize(task + 4u, child, text, kTiming);
    const auto stunt_text = runtime.ActorStuntText(
        runtime.ReadWord(task + 44u), runtime.ReadWord(task + 52u));
    const auto current_stunt = runtime.ReadWord(task + 52u);
    const auto final_target = runtime.ReadWord(task + 48u);
    const auto final_source = runtime.ReadWord(task + 44u);
    runtime.WriteWord(child + 320u, final_target);
    runtime.WriteWord(child + 316u, final_source);
    runtime.WriteWord(child + 324u, current_stunt);
    runtime.WriteWord(child + 60u, final_source);
    const auto result = CombatChildTextAssignment::Assign(
        child + 64u, stunt_text, runtime);
    runtime.WriteByte(child + 332u, 1u);
    return result;
}

std::uint32_t StuntSequenceTask::DestroyBase(
    const std::uint32_t task, IScriptTaskBaseRuntime& runtime) {
    return ScriptTaskBase::Destroy(task, runtime);
}

std::uint32_t StuntSequenceTask::Destroy(
    const std::uint32_t task, const std::uint8_t flags,
    IScriptTaskBaseRuntime& runtime) {
    (void)DestroyBase(task, runtime);
    if ((flags & 1u) != 0u) runtime.Free(task);
    return task;
}

std::uint32_t QuickStuntSequenceTask::DestroyBase(
    const std::uint32_t task, IScriptTaskBaseRuntime& runtime) {
    return ScriptTaskBase::Destroy(task, runtime);
}

std::uint32_t QuickStuntSequenceTask::Destroy(
    const std::uint32_t task, const std::uint8_t flags,
    IScriptTaskBaseRuntime& runtime) {
    (void)DestroyBase(task, runtime);
    if ((flags & 1u) != 0u) runtime.Free(task);
    return task;
}

}  // namespace pal4
