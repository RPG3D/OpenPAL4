#include "pal4/combat_use_property_sequence_task.h"

namespace pal4 {

QuickUsePropertyChildFactorySlot* QuickUsePropertyChildFactory::Initialize(
    QuickUsePropertyChildFactorySlot* const slot) {
    slot->vtable = kVtable;
    return slot;
}

std::uint32_t QuickUsePropertySequenceTask::Start(
    const std::uint32_t task, IQuickUsePropertyStartRuntime& runtime) {
    constexpr std::array<std::uint32_t, 3> timing{1u, 0u, 0u};
    runtime.NotifySource(runtime.ReadWord(task + 44u));
    const auto configured_text = runtime.ReadWord(0x8f9838u);
    const auto text = configured_text == 0u ? 0x841964u
                                             : configured_text;
    QuickUsePropertyChildFactorySlot factory;
    (void)QuickUsePropertyChildFactory::Initialize(&factory);
    const auto child = runtime.CreateChildTask(factory.vtable);
    if (child == 0u) return 0u;
    runtime.AddTaskAndInitialize(task + 4u, child, text, timing);
    const auto property = runtime.ReadWord(task + 52u);
    const auto source = runtime.ReadWord(task + 44u);
    runtime.WriteWord(child + 64u, runtime.ReadWord(task + 48u));
    runtime.WriteWord(child + 60u, source);
    runtime.WriteWord(child + 68u, property);
    return property;
}

namespace {

class IteratorGuard final {
public:
    IteratorGuard(IQuickUsePropertyTransitionRuntime& runtime,
                  const std::uint32_t iterator)
        : runtime_(runtime), iterator_(iterator) {}
    ~IteratorGuard() {
        if (iterator_ != 0u) runtime_.DestroyIterator(iterator_, 1u);
    }
private:
    IQuickUsePropertyTransitionRuntime& runtime_;
    std::uint32_t iterator_;
};

}  // namespace

std::uint32_t QuickUsePropertySequenceTask::CreateNext(
    const std::uint32_t task,
    IQuickUsePropertyTransitionRuntime& runtime) {
    if (!runtime.CombatActive()) return 0u;
    const auto source = runtime.ReadWord(task + 44u);
    if (runtime.SourceStateThree(source)) {
        const auto dead = CombatDeadActionFactory::Create(
            runtime.ReadWord(task + 44u),
            runtime.ReadWord(task + 48u), runtime);
        if (dead == 0u) return 0u;
        runtime.WriteWord(dead + 36u, CombatEndActionFactory::Create(
            runtime.ReadWord(task + 44u), runtime));
        return dead;
    }

    const auto target = runtime.ReadWord(task + 48u);
    const auto iterator = runtime.OpenIterator(target, 464u);
    if (iterator == 0u) return 0u;
    IteratorGuard guard{runtime, iterator};
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
