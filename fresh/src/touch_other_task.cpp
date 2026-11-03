#include "pal4/touch_other_task.h"
#include "pal4/action_task_object.h"
#include "pal4/task_pointer_collection.h"

namespace pal4 {

std::uint32_t TouchOtherTask::Construct(
    const std::uint32_t object, ITouchOtherTaskRuntime& runtime) {
    runtime.BaseConstruct(object);
    runtime.WriteWord(object, 0x8486fcu);
    try {
        runtime.InitializeEmptyString(object + 0x44u);
    } catch (...) {
        (void)runtime.BaseDestruct(object);
        throw;
    }
    runtime.WriteWord(object, kVtable);
    runtime.WriteByte(object + 0x3cu, 0u);
    runtime.WriteWord(object + 0x54u, 0u);
    runtime.WriteWord(object + 0x58u, 0u);
    runtime.WriteWord(object + 0x50u, 0u);
    return object;
}

std::uint32_t TouchOtherTask::Destruct(
    const std::uint32_t object, ITouchOtherTaskRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    runtime.WriteByte(object + 0x3cu, 0u);
    runtime.WriteWord(object + 0x54u, 0u);
    runtime.WriteWord(object + 0x58u, 0u);
    runtime.WriteWord(object + 0x50u, 0u);
    runtime.CleanupString(object + 0x44u);
    runtime.WriteWord(object, 0x8486fcu);
    return runtime.BaseDestruct(object);
}

std::uint8_t TouchOtherTask::Enter(
    const std::uint32_t object, ITouchOtherTaskRuntime& runtime) {
    if (runtime.ReadWord(object + 0x54u) == 0u) return 0u;
    runtime.WriteWord(object + 0x50u, 0u);
    return 1u;
}

std::uint8_t TouchOtherTask::Update(
    const std::uint32_t object, ITouchOtherTaskRuntime& runtime) {
    const auto owner = runtime.ReadWord(object + 0x54u);
    if (owner == 0u) {
        runtime.WriteByte(object + 0x3cu, 0u);
        return 0u;
    }
    if (runtime.ReadByte(object + 0x3cu) != 0u) return 0u;
    const auto target = runtime.FindObjectByName(
        runtime.ReadWord(object + 0x44u));
    if (target != 0u && runtime.ReadWord(object + 0x50u) == 0u) {
        runtime.TouchObject(target, owner + 0xacu);
        runtime.WriteWord(object + 0x50u, 1u);
    }
    if (runtime.ReadWord(object + 0x40u) == 0u ||
        runtime.ObjectDone(target) ||
        runtime.ReadWord(target + 0x184u) == 0u)
        runtime.WriteByte(object + 0x3cu, 1u);
    return 1u;
}

std::uint8_t TouchOtherTask::SpawnFollowup(
    const std::uint32_t object, ITouchOtherTaskSpawnRuntime& runtime) {
    const auto context = runtime.ReadWord(object + 0x58u);
    if (!context) return 0u;
    const auto owner = runtime.ReadWord(object + 0x54u);
    if (!owner) return 0u;
    const auto previous = runtime.ReadWord(object + 0x30u);
    if (previous) {
        runtime.NotifyPreviousTask(previous);
        return 1u;
    }
    const auto helper = ActionTaskFactory::CreateHelper(runtime);
    if (helper) runtime.AddHelperTask(context, helper, 1u, 0u, 0u);
    runtime.SetHelperOwner(helper, owner);
    TaskPointerCollection::Append(owner + 0x2a0u, helper, runtime);
    runtime.MarkProcessed(context);
    return 1u;
}

std::uint32_t TouchOtherTask::Initialize(
    const std::uint32_t object, const std::uint32_t owner,
    const std::uint32_t object_name, const std::uint32_t wait,
    const std::uint32_t context, ITouchOtherTaskRuntime& runtime) {
    runtime.WriteWord(object + 0x54u, owner);
    runtime.WriteWord(object + 0x40u, wait);
    runtime.WriteByte(object + 0x3cu, 0u);
    const auto result = runtime.InitializeString(
        object + 0x44u, object_name, runtime.CStringLength(object_name));
    runtime.WriteWord(object + 0x58u, context);
    return result;
}

}  // namespace pal4
