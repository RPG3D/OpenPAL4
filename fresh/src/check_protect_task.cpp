#include "pal4/check_protect_task.h"

namespace pal4 {
namespace {

std::uint32_t FirstReadyOther(
    const std::uint32_t collection, const std::uint32_t virtual_offset,
    const std::uint32_t actor, ICheckProtectTaskRuntime& runtime) {
    const auto iterator = runtime.OpenIterator(collection, virtual_offset);
    struct Guard {
        ICheckProtectTaskRuntime& runtime;
        std::uint32_t iterator;
        ~Guard() {
            if (iterator != 0u) runtime.DeleteIterator(iterator, 1u);
        }
    } guard{runtime, iterator};
    while (!runtime.IteratorAtEnd(iterator)) {
        const auto candidate = runtime.IteratorCurrent(iterator);
        if (runtime.IsActorReady(candidate) && candidate != actor)
            return candidate;
        runtime.IteratorNext(iterator);
    }
    return 0u;
}

}  // namespace

std::uint32_t CheckProtectTask::Create(
    const std::uint32_t owner, const std::uint32_t actor,
    ICheckProtectTaskRuntime& runtime) {
    if (!runtime.IsActorReady(actor)) return 0u;

    auto protector = runtime.ReadWord(owner + 48u);
    if (!runtime.IsActorReady(protector))
        protector = FirstReadyOther(protector, 464u, actor, runtime);
    if (protector == 0u) return 0u;

    if (actor == protector ||
        runtime.ReadWord(actor + 804u) ==
            runtime.ReadWord(protector + 804u)) {
        protector = FirstReadyOther(runtime.ReadWord(owner + 44u),
                                    472u, actor, runtime);
    }
    if (protector == 0u) return 0u;

    const auto task = runtime.Allocate(kSize);
    if (task == 0u) return 0u;
    struct TaskGuard {
        ICheckProtectTaskRuntime& runtime;
        std::uint32_t task;
        bool initialized = false;
        bool name_initialized = false;
        ~TaskGuard() {
            if (task == 0u) return;
            if (name_initialized) runtime.CleanupName(task + 20u);
            if (initialized) runtime.DestroyContainer(task + 4u);
            runtime.Free(task);
        }
    } guard{runtime, task};

    const auto mode = runtime.ReadByte(owner + 56u);
    runtime.InitializeContainer(task + 4u);
    guard.initialized = true;
    runtime.WriteByte(task + 20u,
                      static_cast<std::uint8_t>(actor));
    runtime.WriteWord(task + 24u, 0u);
    runtime.WriteWord(task + 28u, 0u);
    runtime.WriteWord(task + 32u, 0u);
    guard.name_initialized = true;
    runtime.AssignName(task + 20u, "CheckProtect");
    runtime.WriteWord(task + 36u, 0u);
    runtime.WriteWord(task + 52u, 0u);
    runtime.WriteByte(task + 40u, 1u);
    runtime.WriteWord(task + 44u, actor);
    runtime.WriteWord(task + 48u, protector);
    runtime.WriteByte(task + 56u, mode);
    runtime.WriteWord(task, kVtable);
    guard.task = 0u;
    return task;
}

std::uint32_t CheckProtectTask::Start(
    const std::uint32_t task, ICheckProtectStartRuntime& runtime,
    IScriptActorEventRuntime& actor_runtime) {
    class Selection final : public ICombatProtectionSelectionRuntime {
        IScriptActorEventRuntime& runtime;
    public:
        explicit Selection(IScriptActorEventRuntime& r):runtime(r){}
        std::uint32_t Select(std::uint32_t source,std::uint32_t target) override{
            return ScriptActorEventProcessor::SelectTarget(source,target,runtime);
        }
    } selection(actor_runtime);
    return Start(task,runtime,selection);
}

std::uint32_t CheckProtectTask::Start(
    const std::uint32_t task, ICheckProtectStartRuntime& runtime,
    ICombatProtectionSelectionRuntime& selection) {
    const auto source = runtime.ReadWord(task + 44u);
    runtime.NotifySource(source);
    const auto protector = runtime.ReadWord(task + 48u);
    const auto target = selection.Select(source, protector);
    runtime.WriteWord(task + 52u, target);
    if (target == 0u) return 0u;

    std::array<float, 3> position;
    (void)CalculatePosition(task, position, runtime);
    constexpr std::array<std::uint32_t, 3> timing{1u, 0u, 0u};
    CheckProtectSubtaskFactorySlot factory;
    (void)CheckProtectSubtaskFactory::InitializePosition(&factory);
    const auto first = runtime.CreateChildTask(factory);
    if (first == 0u) return 0u;
    runtime.AddTaskAndInitialize(task + 4u, first,
                                 runtime.GlobalText(0x8f9798u), timing);
    runtime.WriteWord(first + 72u, target);
    for (std::uint32_t i = 0u; i < 3u; ++i)
        runtime.WriteFloat(first + 60u + i * 4u, position[i]);

    (void)CheckProtectSubtaskFactory::InitializeResponse(&factory);
    const auto second = runtime.CreateChildTask(factory);
    if (second == 0u) return 0u;
    runtime.AddTaskAndInitialize(task + 4u, second,
                                 runtime.GlobalText(0x8f9634u), timing);
    runtime.WriteWord(second + 68u, protector);
    runtime.WriteWord(second + 60u, source);
    runtime.WriteWord(second + 64u, target);
    return protector;
}

std::array<float, 3>* CheckProtectTask::CalculatePosition(
    const std::uint32_t task, std::array<float, 3>& output,
    ICheckProtectPositionRuntime& runtime) {
    const auto source = runtime.ReadWord(task + 44u);
    const auto source_position = runtime.ActorChildPosition(source);
    const auto protector = runtime.ReadWord(task + 48u);
    const auto protector_position = runtime.ActorChildPosition(protector);
    std::array<float, 3> direction{
        source_position[0] - protector_position[0],
        source_position[1] - protector_position[1],
        source_position[2] - protector_position[2]};
    runtime.NormalizeVector(direction);
    const auto bounds = runtime.ActorChildBoundingBox(source);
    const auto distance = (bounds[0] - bounds[3]) * 0.5f + 50.0f;
    for (std::uint32_t i = 0u; i < 3u; ++i)
        output[i] = protector_position[i] + direction[i] * distance;
    return &output;
}

CheckProtectSubtaskFactorySlot*
CheckProtectSubtaskFactory::InitializePosition(
    CheckProtectSubtaskFactorySlot* const slot) {
    slot->vtable = kPositionVtable;
    return slot;
}

CheckProtectSubtaskFactorySlot*
CheckProtectSubtaskFactory::InitializeResponse(
    CheckProtectSubtaskFactorySlot* const slot) {
    slot->vtable = kResponseVtable;
    return slot;
}

}  // namespace pal4
