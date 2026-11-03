#include "pal4/combat_fatal_sequence_runtime.h"

#include "pal4/combat_dead_sequence_task.h"
#include "pal4/combat_wounded_sequence_task.h"

namespace pal4 {
namespace {

constexpr std::uint32_t kEmptyText = 0x841964u;
constexpr std::array<std::uint32_t, 3> kNormalTiming{1u, 0u, 0u};
constexpr std::array<std::uint32_t, 3> kFinishTiming{
    1u, 0x3fc00000u, 0x3f800000u};

std::uint32_t TextOrEmpty(IFatalSequenceRuntime& runtime,
                          std::uint32_t address) {
    const auto text = runtime.ReadWord(address);
    return text == 0u ? kEmptyText : text;
}

std::uint32_t AddChild(std::uint32_t task,
                       std::uint32_t factory_vtable,
                       std::uint32_t text,
                       const std::array<std::uint32_t, 3>& timing,
                       IFatalSequenceRuntime& runtime) {
    const auto child = runtime.CreateChildTask(factory_vtable);
    if (child != 0u)
        runtime.AddTaskAndInitialize(task + 4u, child, text, timing);
    return child;
}

FatalChildFactorySlot* Initialize(FatalChildFactorySlot* slot,
                                  std::uint32_t vtable) {
    slot->vtable = vtable;
    return slot;
}

bool AddCollapse(std::uint32_t task, std::uint32_t text_source,
                 IFatalSequenceRuntime& runtime) {
    const auto child = AddChild(task, 0x845d34u,
                                TextOrEmpty(runtime, 0x8f9620u),
                                kNormalTiming, runtime);
    if (child == 0u) return false;
    runtime.WriteWord(child + 60u, runtime.ReadWord(task + 44u));
    runtime.AssignText(child + 64u, text_source);
    runtime.WriteByte(child + 80u, 0u);
    return true;
}

bool AddFinalPair(std::uint32_t task, IFatalSequenceRuntime& runtime) {
    auto child = AddChild(task, 0x845de4u,
                          TextOrEmpty(runtime, 0x8f9748u),
                          kNormalTiming, runtime);
    if (child == 0u) return false;
    runtime.WriteWord(child + 60u, runtime.ReadWord(task + 44u));
    child = AddChild(task, 0x845de8u,
                     TextOrEmpty(runtime, 0x8f975cu),
                     kFinishTiming, runtime);
    if (child != 0u)
        runtime.WriteWord(child + 60u, runtime.ReadWord(task + 44u));
    return child != 0u;
}

}  // namespace

FatalChildFactorySlot* FatalChildFactories::InitializeEvent(
    FatalChildFactorySlot* slot) {
    return Initialize(slot, 0x845de0u);
}
FatalChildFactorySlot* FatalChildFactories::InitializeFirstFinal(
    FatalChildFactorySlot* slot) {
    return Initialize(slot, 0x845de4u);
}
FatalChildFactorySlot* FatalChildFactories::InitializeFinish(
    FatalChildFactorySlot* slot) {
    return Initialize(slot, 0x845de8u);
}
FatalChildFactorySlot* FatalChildFactories::InitializeEnd(
    FatalChildFactorySlot* slot) {
    return Initialize(slot, 0x845decu);
}
FatalChildFactorySlot* FatalChildFactories::InitializeMessage(
    FatalChildFactorySlot* slot) {
    return Initialize(slot, 0x845df0u);
}

void DeadSequenceTask::Start(std::uint32_t task,
                             IFatalSequenceRuntime& runtime) {
    const auto actor = runtime.ReadWord(task + 44u);
    if (actor == 0u || runtime.ReadWord(task + 48u) == 0u) return;
    runtime.SetActorState(actor, 3u, 1u);

    auto child = AddChild(task, 0x845de0u,
                          TextOrEmpty(runtime, 0x8f965cu),
                          kNormalTiming, runtime);
    if (child == 0u) return;
    runtime.WriteWord(child + 64u, runtime.EventDataByIndex(43u));
    runtime.WriteWord(child + 60u, runtime.ReadWord(task + 44u));
    runtime.WriteWord(child + 68u, 0x40400000u);

    if (!AddCollapse(task, 0x8fa35cu, runtime)) return;
    if (runtime.ReadWord(runtime.ReadWord(task + 44u) + 804u) == 2u &&
        !AddFinalPair(task, runtime))
        return;

    child = AddChild(task, 0x845decu,
                     TextOrEmpty(runtime, 0x8f96d0u),
                     kNormalTiming, runtime);
    if (child != 0u) {
        runtime.WriteWord(child + 60u, runtime.ReadWord(task + 44u));
        runtime.WriteWord(child + 64u, runtime.ReadWord(task + 48u));
    }
}

void FinalHitSequenceTask::Start(std::uint32_t task,
                                 IFatalSequenceRuntime& runtime) {
    const auto actor = runtime.ReadWord(task + 44u);
    if (actor == 0u || runtime.ReadWord(task + 48u) == 0u) return;
    runtime.SetActorState(actor, 2u, 0u);
    if (!AddCollapse(task, 0x8fa3a0u, runtime)) return;
    const auto source = runtime.ReadWord(task + 44u);
    if (runtime.ReadWord(source + 804u) != 2u) return;

    if (runtime.ReadWord(source + 1056u) != 0u) {
        const auto child = AddChild(task, 0x845df0u,
                                    TextOrEmpty(runtime, 0x8f9734u),
                                    kNormalTiming, runtime);
        if (child == 0u) return;
        const auto message = runtime.ReadWord(source + 1052u);
        runtime.WriteWord(child + 76u,
                          runtime.ReadWord(task + 44u));
        runtime.WriteByte(child + 88u, 0u);
        runtime.SetMessage(child + 60u,
                           message == 0u ? kEmptyText : message);
    }
    (void)AddFinalPair(task, runtime);
}

std::uint32_t FinalHitSequenceTask::CreateNext(
    std::uint32_t task, IFinalHitCompletionRuntime& runtime) {
    runtime.ActorVirtual172(runtime.ReadWord(task + 44u), 3u);
    const auto actor = runtime.ReadWord(task + 44u);
    const auto target = runtime.ReadWord(task + 48u);
    return runtime.ActorVirtual544(actor, target);
}

void QuickDeadSequenceTask::Start(
    std::uint32_t task, IFatalSequenceRuntime& runtime) {
    const auto actor = runtime.ReadWord(task + 44u);
    if (actor == 0u || runtime.ReadWord(task + 48u) == 0u) return;
    runtime.SetActorState(actor, 3u, 1u);
    auto child = AddChild(task, 0x845de0u,
                          TextOrEmpty(runtime, 0x8f965cu),
                          kNormalTiming, runtime);
    if (child == 0u) return;
    runtime.WriteWord(child + 64u, runtime.EventDataByIndex(43u));
    runtime.WriteWord(child + 60u, runtime.ReadWord(task + 44u));
    runtime.WriteWord(child + 68u, 0x40400000u);
    child = AddChild(task, 0x845decu,
                     TextOrEmpty(runtime, 0x8f96d0u),
                     kNormalTiming, runtime);
    if (child != 0u) {
        runtime.WriteWord(child + 60u, runtime.ReadWord(task + 44u));
        runtime.WriteWord(child + 64u, runtime.ReadWord(task + 48u));
    }
}

}  // namespace pal4
