#include "pal4/time_wait_task.h"

#include "pal4/action_task_object.h"
#include "pal4/task_pointer_collection.h"

#include <bit>

namespace pal4 {

std::uint32_t TimeWaitTask::Construct(
    const std::uint32_t object, ITimeWaitTaskRuntime& runtime) {
    runtime.BaseConstruct(object);
    runtime.WriteWord(object, kVtable);
    runtime.WriteWord(object + 0x48u, 0u);
    runtime.WriteWord(object + 0x4cu, 0u);
    return object;
}

std::uint32_t TimeWaitTask::Destruct(
    const std::uint32_t object, ITimeWaitTaskRuntime& runtime) {
    runtime.WriteWord(object, 0x8486fcu);
    runtime.WriteWord(object + 0x48u, 0u);
    runtime.WriteWord(object + 0x4cu, 0u);
    return runtime.BaseDestruct(object);
}

bool TimeWaitTask::Enter(
    const std::uint32_t object, ITimeWaitTaskRuntime& runtime) {
    return runtime.ReadWord(object + 0x48u) != 0u;
}

std::uint8_t TimeWaitTask::Update(
    const std::uint32_t object, const float delta_seconds,
    ITimeWaitTaskRuntime& runtime) {
    if (runtime.ReadWord(object + 0x48u) == 0u) {
        runtime.WriteByte(object + 0x3cu, 0u);
        return 0u;
    }
    if (runtime.ReadByte(object + 0x3cu) != 0u) return 0u;
    // The game's x87 PC24 arithmetic rounds this addition to float already.
    const float elapsed = std::bit_cast<float>(runtime.ReadWord(object + 0x44u)) +
                          delta_seconds;
    runtime.WriteWord(object + 0x44u,
                      std::bit_cast<std::uint32_t>(elapsed));
    //599936..940: equality (C3) keeps the task incomplete.
    if (elapsed > std::bit_cast<float>(
                       runtime.ReadWord(object + 0x40u)))
        runtime.WriteByte(object + 0x3cu, 1u);
    return 1u;
}

std::uint8_t TimeWaitTask::SpawnFollowup(
    const std::uint32_t object, ITimeWaitTaskSpawnRuntime& runtime) {
    const auto context = runtime.ReadWord(object + 0x4cu);
    if (context == 0u) return 0u;
    const auto owner = runtime.ReadWord(object + 0x48u);
    if (owner == 0u) return 0u;
    const auto child = TimeWaitTaskFactory::Create(runtime);
    if (child != 0u)
        runtime.AddReverseTask(context, child, object + 0x0cu);
    (void)Initialize(child, owner, runtime.ReadWord(object + 0x40u),
                     0u, runtime);
    runtime.InsertOwnerTask(owner + 0x2a0u, child);
    const auto previous = runtime.ReadWord(object + 0x30u);
    if (previous != 0u) {
        runtime.NotifyPreviousTask(previous);
        return 1u;
    }
    const auto helper = ActionTaskFactory::CreateHelper(runtime);
    if (helper != 0u)
        runtime.AddHelperTask(context, helper, 1u, 0u, 0u);
    runtime.SetHelperOwner(helper, owner);
    TaskPointerCollection::Append(owner + 0x2a0u, helper, runtime);
    runtime.MarkProcessed(context);
    return 1u;
}

std::uint32_t TimeWaitTaskFactory::ConstructDescriptor(
    const std::uint32_t descriptor, ITimeWaitTaskRuntime& runtime) {
    runtime.WriteWord(descriptor, kDescriptorVtable);
    return descriptor;
}

std::uint32_t TimeWaitTask::Initialize(
    const std::uint32_t object, const std::uint32_t owner,
    const std::uint32_t duration_bits, const std::uint32_t context,
    ITimeWaitTaskRuntime& runtime) {
    runtime.WriteByte(object + 0x3cu, 0u);
    runtime.WriteWord(object + 0x48u, owner);
    runtime.WriteWord(object + 0x44u, 0u);
    runtime.WriteWord(object + 0x40u, duration_bits);
    runtime.WriteWord(object + 0x4cu, context);
    return context;
}

std::uint32_t TimeWaitTaskFactory::Create(
    ITimeWaitTaskSpawnRuntime& runtime) {
    const auto memory = runtime.AllocateMemory(0x50u);
    if (memory == 0u) return 0u;
    try {
        return TimeWaitTask::Construct(memory, runtime);
    } catch (...) {
        runtime.FreeMemory(memory);
        throw;
    }
}

}  // namespace pal4
