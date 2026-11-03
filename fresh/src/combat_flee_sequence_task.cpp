#include "pal4/combat_flee_sequence_task.h"

#include "pal4/script_task_base.h"
#include "pal4/combat_miss_sequence_task.h"

#include <limits>

namespace pal4 {
namespace {

class FleeIteratorGuard final {
public:
    FleeIteratorGuard(IFleeSequenceRuntime& runtime,
                      const std::uint32_t iterator)
        : runtime_(runtime), iterator_(iterator) {}

    ~FleeIteratorGuard() {
        if (iterator_ != 0u)
            (void)runtime_.DestroyActorIterator(iterator_, 1u);
    }

    [[nodiscard]] std::uint32_t Close() {
        if (iterator_ == 0u) return 0u;
        const auto iterator = iterator_;
        iterator_ = 0u;
        return runtime_.DestroyActorIterator(iterator, 1u);
    }

private:
    IFleeSequenceRuntime& runtime_;
    std::uint32_t iterator_;
};

std::uint32_t TextOrEmpty(IFleeSequenceRuntime& runtime,
                          const std::uint32_t address) {
    const auto value = runtime.ReadWord(address);
    return value == 0u ? 0x841964u : value;
}

std::uint32_t CreateFinishTask(
    const std::uint32_t task, const bool chance,
    const std::array<std::uint32_t, 3>& timing,
    IFleeSequenceRuntime& runtime, bool* const created = nullptr) {
    if (created != nullptr) *created = false;
    const auto text = TextOrEmpty(runtime, 0x8f96e4u);
    FleeChildFactorySlot factory;
    (void)FleeFinishChildFactory::Initialize(&factory);
    const auto child = runtime.CreateChildTask(factory.vtable);
    if (child == 0u) return 0u;
    if (created != nullptr) *created = true;
    const auto result = runtime.AddTaskAndInitialize(
        task + 4u, child, text, timing);
    const auto actor = runtime.ReadWord(task + 44u);
    runtime.WriteByte(child + 64u, chance ? 1u : 0u);
    runtime.WriteWord(child + 60u, actor);
    return result;
}

}  // namespace

FleeChildFactorySlot* FleeCandidateChildFactory::Initialize(
    FleeChildFactorySlot* const slot) {
    slot->vtable = kVtable;
    return slot;
}

FleeChildFactorySlot* FleeFinishChildFactory::Initialize(
    FleeChildFactorySlot* const slot) {
    slot->vtable = kVtable;
    return slot;
}

bool FleeSequenceTask::CheckPartyChance(
    const std::uint32_t task, IFleeSequenceRuntime& runtime) {
    const auto iterator = runtime.CreateActorIterator(
        runtime.ReadWord(task + 44u), 472u);
    if (iterator == 0u) return false;
    FleeIteratorGuard guard{runtime, iterator};

    std::uint32_t count = 0u;
    float sum = 0.0f;
    bool any_flagged_actor = false;
    while (!runtime.IteratorAtEnd(iterator)) {
        const auto actor = runtime.IteratorValue(iterator);
        const auto property = runtime.ActorPropertySix(actor);
        sum = static_cast<float>(static_cast<double>(sum) + property);
        if (runtime.ActorFlag412(actor)) any_flagged_actor = true;
        runtime.IteratorNext(iterator);
        ++count;
    }

    const auto average = count == 0u
        ? std::numeric_limits<float>::quiet_NaN()
        : sum / static_cast<float>(count);
    const auto source = runtime.ReadWord(task + 44u);
    const auto value = runtime.ActorPropertySix(source);
    const auto critical = runtime.CheckCriticalHit(
        static_cast<float>(value), average);
    const auto chance = critical && !any_flagged_actor &&
                        !runtime.CombatFlag501();
    (void)guard.Close();
    return chance;
}

std::uint32_t FleeSequenceTask::Start(
    const std::uint32_t task, IFleeSequenceRuntime& runtime) {
    constexpr std::array<std::uint32_t, 3> base_timing{1u, 0u, 0u};
    runtime.NotifyActor(runtime.ReadWord(task + 44u));
    const auto chance = CheckPartyChance(task, runtime);
    const auto actor = runtime.ReadWord(task + 44u);
    if (runtime.ReadWord(actor + 804u) != 1u) {
        constexpr std::array<std::uint32_t, 3> timing{
            1u, 0u, 0x3f800000u};
        return CreateFinishTask(task, chance, timing, runtime);
    }

    if (!chance) {
        const auto text = TextOrEmpty(runtime, 0x8f960cu);
        MissMessageChildFactorySlot factory;
        (void)MissMessageChildFactory::Initialize(&factory);
        const auto child = runtime.CreateChildTask(factory.vtable);
        if (child == 0u) return 0u;
        (void)runtime.AddTaskAndInitialize(
            task + 4u, child, text, base_timing);
        runtime.WriteWord(child + 60u, runtime.ReadWord(task + 44u));
        runtime.AssignText(child + 64u, 0x8fa298u, 0u, 0xffffffffu);
        return CreateFinishTask(task, false, base_timing, runtime);
    }

    const auto iterator = runtime.CreateActorIterator(actor, 464u);
    if (iterator == 0u) return 0u;
    FleeIteratorGuard guard{runtime, iterator};
    while (!runtime.IteratorAtEnd(iterator)) {
        const auto candidate = runtime.IteratorValue(iterator);
        if (runtime.IsActorReady(candidate)) {
            const auto text = TextOrEmpty(runtime, 0x8f9620u);
            FleeChildFactorySlot factory;
            (void)FleeCandidateChildFactory::Initialize(&factory);
            const auto child = runtime.CreateChildTask(factory.vtable);
            if (child == 0u) return 0u;
            (void)runtime.AddTaskAndInitialize(
                task + 4u, child, text, base_timing);
            runtime.WriteWord(child + 60u, candidate);
            runtime.AssignText(child + 64u, 0x8fa2a8u, 0u, 0xffffffffu);
            runtime.WriteByte(child + 80u, 0u);
        }
        runtime.IteratorNext(iterator);
    }

    constexpr std::array<std::uint32_t, 3> finish_timing{
        1u, 0x40000000u, 0u};
    bool finish_created = false;
    (void)CreateFinishTask(task, true, finish_timing, runtime,
                           &finish_created);
    if (!finish_created)
        return 0u;
    return guard.Close();
}

std::uint32_t QuickFleeSequenceTask::Start(
    const std::uint32_t task, IFleeSequenceRuntime& runtime) {
    constexpr std::array<std::uint32_t, 3> timing{1u, 0u, 0u};
    runtime.NotifyActor(runtime.ReadWord(task + 44u));
    const auto chance = FleeSequenceTask::CheckPartyChance(task, runtime);
    return CreateFinishTask(task, chance, timing, runtime);
}

std::uint32_t FleeSequenceTask::DestroyBase(
    const std::uint32_t task, IScriptTaskBaseRuntime& runtime) {
    return ScriptTaskBase::Destroy(task, runtime);
}

std::uint32_t FleeSequenceTask::Destroy(
    const std::uint32_t task, const std::uint8_t flags,
    IScriptTaskBaseRuntime& runtime) {
    (void)DestroyBase(task, runtime);
    if ((flags & 1u) != 0u) runtime.Free(task);
    return task;
}

std::uint32_t QuickFleeSequenceTask::DestroyBase(
    const std::uint32_t task, IScriptTaskBaseRuntime& runtime) {
    return ScriptTaskBase::Destroy(task, runtime);
}

std::uint32_t QuickFleeSequenceTask::Destroy(
    const std::uint32_t task, const std::uint8_t flags,
    IScriptTaskBaseRuntime& runtime) {
    (void)DestroyBase(task, runtime);
    if ((flags & 1u) != 0u) runtime.Free(task);
    return task;
}

}  // namespace pal4
