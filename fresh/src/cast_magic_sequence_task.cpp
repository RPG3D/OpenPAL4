#include "pal4/cast_magic_sequence_task.h"

#include "pal4/script_task_base.h"
#include "pal4/start_attack_task.h"

namespace pal4 {

std::uint32_t CastMagicSequenceTask::DestroyBase(
    const std::uint32_t task, IScriptTaskBaseRuntime& runtime) {
    return ScriptTaskBase::Destroy(task, runtime);
}

std::uint32_t CastMagicSequenceTask::Destroy(
    const std::uint32_t task, const std::uint8_t flags,
    IScriptTaskBaseRuntime& runtime) {
    (void)DestroyBase(task, runtime);
    if ((flags & 1u) != 0u) runtime.Free(task);
    return task;
}

namespace {

constexpr std::uint32_t kEmptyText = 0x841964u;
constexpr std::array<std::uint32_t, 3> kTiming{1u, 0u, 0u};

}  // namespace

CastMagicChildFactorySlot* CastMagicChildFactory::Initialize(
    CastMagicChildFactorySlot* const slot) {
    slot->vtable = kVtable;
    return slot;
}

std::uint8_t CastMagicSequenceTask::Start(
    const std::uint32_t task, ICastMagicSequenceRuntime& runtime) {
    const auto target = runtime.ReadWord(task + 48u);
    const auto child_position = runtime.ActorChildPosition(target);
    runtime.NotifySource(runtime.ReadWord(task + 44u));

    if (runtime.ReadWord(task + 44u) !=
        runtime.ReadWord(task + 48u)) {
        const auto magic = runtime.ReadWord(task + 52u);
        const auto mode = runtime.MagicMode(magic);
        const auto configured_text = runtime.ReadWord(0x8f9784u);
        const auto text = configured_text == 0u ? kEmptyText
                                                  : configured_text;
        StartAttackApproachFactorySlot factory;
        (void)StartAttackApproachFactory::InitializePrimary(&factory);
        const auto run = runtime.CreateChildTask(factory.vtable);
        if (run == 0u) return 0u;
        runtime.AddTaskAndInitialize(task + 4u, run, text, kTiming);
        const auto position = mode == 3u || mode == 4u
            ? runtime.ActorTargetPosition(runtime.ReadWord(task + 48u))
            : child_position;
        runtime.SetApproachTaskTarget(run, runtime.ReadWord(task + 44u), position);
    }

    const auto configured_text = runtime.ReadWord(0x8f970cu);
    const auto text = configured_text == 0u ? kEmptyText
                                              : configured_text;
    CastMagicChildFactorySlot factory;
    (void)CastMagicChildFactory::Initialize(&factory);
    const auto child = runtime.CreateChildTask(factory.vtable);
    if (child == 0u) return 0u;
    runtime.AddTaskAndInitialize(task + 4u, child, text, kTiming);
    const auto cast_source = runtime.ReadWord(task + 44u);
    const auto cast_magic = runtime.ReadWord(task + 52u);
    const auto cast_text = runtime.ActorCastText(cast_source, cast_magic);
    const auto current_magic = runtime.ReadWord(task + 52u);
    const auto current_target = runtime.ReadWord(task + 48u);
    const auto current_source = runtime.ReadWord(task + 44u);
    runtime.WriteWord(child + 320u, current_target);
    runtime.WriteWord(child + 316u, current_source);
    runtime.WriteWord(child + 324u, current_magic);
    runtime.WriteWord(child + 60u, current_source);
    const auto result = CombatChildTextAssignment::Assign(
        child + 64u, cast_text, runtime);
    runtime.WriteByte(child + 332u, 1u);
    return result;
}

}  // namespace pal4
