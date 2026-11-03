#include "pal4/combat_stunt_sequence_task.h"

#include <string_view>

namespace pal4 {
namespace {

std::uint32_t CreateDeadThenEnd(
    const std::uint32_t task, IStuntTransitionRuntime& runtime) {
    const auto dead = runtime.CreateDeadAction(
        runtime.ReadWord(task + 44u),
        runtime.ReadWord(task + 48u));
    if (dead == 0u) return 0u;
    const auto end = CombatEndActionFactory::Create(
        runtime.ReadWord(task + 44u), runtime);
    runtime.AttachDeadSuccessor(dead, end);
    return dead;
}

class QuickStuntIteratorGuard final {
public:
    QuickStuntIteratorGuard(IQuickStuntTransitionRuntime& runtime,
                            const std::uint32_t iterator)
        : runtime_(runtime), iterator_(iterator) {}
    ~QuickStuntIteratorGuard() {
        if (iterator_ != 0u) runtime_.DestroyIterator(iterator_, 1u);
    }
private:
    IQuickStuntTransitionRuntime& runtime_;
    std::uint32_t iterator_;
};

}  // namespace

std::uint32_t StuntSequenceTask::CreateAction(
    const std::uint32_t task, const std::uint32_t source,
    IStuntActionRuntime& runtime) {
    if (!runtime.IsActorReady(source)) return 0u;

    auto target = runtime.ReadWord(task + 48u);
    if (!runtime.IsActorReady(target))
        target = CombatReadyTargetScan::FirstReadyOther(
            runtime, target, 464u, source);
    if (target == 0u) return 0u;

    const auto stunt = runtime.ReadWord(task + 52u);
    if (runtime.CanUseStunt(source, stunt) &&
        runtime.ActorPropertySlot8(source, 2u))
        return runtime.StuntAction(runtime.ReadWord(task + 52u),
                                   source, target);

    if (source == target || runtime.ReadWord(source + 804u) ==
                                runtime.ReadWord(target + 804u))
        target = CombatReadyTargetScan::FirstReadyOther(
            runtime, runtime.ReadWord(task + 44u), 472u, source);
    if (target == 0u) return 0u;
    return runtime.NormalAction(source, target, 0u);
}

std::uint32_t StuntSequenceTask::CreateNext(
    const std::uint32_t task, IStuntTransitionRuntime& runtime) {
    const auto source = runtime.ReadWord(task + 44u);
    if (runtime.SourceStateThree(source))
        return CreateDeadThenEnd(task, runtime);

    const auto object = runtime.Allocate(EndStuntSequenceTask::kSize);
    if (object == 0u) return 0u;
    struct ConstructionGuard {
        IStuntTransitionRuntime& runtime;
        std::uint32_t object;
        bool container_initialized = false;
        bool name_initialized = false;
        ~ConstructionGuard() {
            if (object == 0u) return;
            if (name_initialized) runtime.CleanupName(object + 20u);
            if (container_initialized)
                runtime.DestroyContainer(object + 4u);
            runtime.Free(object);
        }
    } guard{runtime, object};

    const auto target = runtime.ReadWord(task + 48u);
    const auto stunt = runtime.ReadWord(task + 52u);
    const auto current_source = runtime.ReadWord(task + 44u);
    runtime.InitializeContainer(object + 4u);
    guard.container_initialized = true;
    // The native constructor copies an unwritten stack byte here.
    runtime.WriteByte(object + 20u, 0u);
    guard.name_initialized = true;
    runtime.WriteWord(object + 24u, 0u);
    runtime.WriteWord(object + 28u, 0u);
    runtime.WriteWord(object + 32u, 0u);
    runtime.SetNameErrorFlag(object + 20u, 1u);
    constexpr std::string_view name = "EndStunt";
    runtime.ReallocateName(object + 20u,
                           static_cast<std::uint32_t>(name.size()));
    runtime.CopyName(object + 20u, name);
    runtime.SetNameSize(object + 20u,
                        static_cast<std::uint32_t>(name.size()));
    runtime.WriteWord(object + 48u, target);
    runtime.WriteWord(object + 36u, 0u);
    runtime.WriteByte(object + 40u, 1u);
    runtime.WriteWord(object + 44u, current_source);
    runtime.WriteWord(object + 52u, stunt);
    runtime.WriteWord(object, EndStuntSequenceTask::kVtable);
    guard.object = 0u;
    return object;
}

std::uint32_t QuickStuntSequenceTask::CreateNext(
    const std::uint32_t task,
    IQuickStuntTransitionRuntime& runtime) {
    if (!runtime.CombatActive()) return 0u;
    const auto source = runtime.ReadWord(task + 44u);
    if (runtime.SourceStateThree(source))
        return CreateDeadThenEnd(task, runtime);

    const auto target = runtime.ReadWord(task + 48u);
    const auto iterator = runtime.OpenIterator(target, 464u);
    if (iterator == 0u) return 0u;
    QuickStuntIteratorGuard guard{runtime, iterator};
    while (!runtime.IteratorAtEnd(iterator)) {
        const auto actor = runtime.IteratorCurrent(iterator);
        if (!runtime.IsActorReady(actor))
            runtime.NotifyUnreadyActor(actor,
                                       runtime.ReadWord(task + 44u));
        runtime.IteratorNext(iterator);
    }
    return CombatEndActionFactory::Create(
        runtime.ReadWord(task + 44u), runtime);
}

}  // namespace pal4
