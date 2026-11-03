#include "pal4/start_task.h"

#include "pal4/action_task_object.h"
#include "pal4/task_pointer_collection.h"

#include <bit>
#include <initializer_list>

namespace pal4 {
namespace {

MoveTaskVector ReadVector(const std::uint32_t address,
                          IStartTaskRuntime& runtime) {
    return {std::bit_cast<float>(runtime.ReadWord(address)),
            std::bit_cast<float>(runtime.ReadWord(address + 4u)),
            std::bit_cast<float>(runtime.ReadWord(address + 8u))};
}

void WriteVector(const std::uint32_t address,
                 const MoveTaskVector& value, IStartTaskRuntime& runtime) {
    runtime.WriteWord(address, std::bit_cast<std::uint32_t>(value.x));
    runtime.WriteWord(address + 4u, std::bit_cast<std::uint32_t>(value.y));
    runtime.WriteWord(address + 8u, std::bit_cast<std::uint32_t>(value.z));
}

}  // namespace

std::uint32_t StartTask::Construct(
    const std::uint32_t object, IStartTaskRuntime& runtime) {
    runtime.BaseConstruct(object);
    runtime.WriteWord(object, kVtable);
    runtime.WriteByte(object + 0x3cu, 0u);
    runtime.WriteWord(object + 0x58u, 0u);
    runtime.WriteWord(object + 0x5cu, 0u);
    return object;
}

std::uint32_t StartTask::Destruct(
    const std::uint32_t object, IStartTaskRuntime& runtime) {
    runtime.WriteWord(object, 0x8486fcu);
    runtime.WriteByte(object + 0x3cu, 0u);
    runtime.WriteWord(object + 0x58u, 0u);
    runtime.WriteWord(object + 0x5cu, 0u);
    return runtime.BaseDestruct(object);
}

std::uint8_t StartTask::Enter(
    const std::uint32_t object, IStartTaskRuntime& runtime) {
    const auto owner = runtime.ReadWord(object + 0x58u);
    if (owner == 0u) return 0u;
    runtime.SetOwnerPosition(owner, ReadVector(object + 0x40u, runtime), 0u);
    runtime.SetOwnerRotation(owner, ReadVector(object + 0x4cu, runtime), 0u);
    runtime.WriteByte(object + 0x3cu, 1u);
    return 1u;
}

std::uint8_t StartTask::Update(
    const std::uint32_t object, IStartTaskRuntime& runtime) {
    if (runtime.ReadWord(object + 0x58u) != 0u)
        return runtime.ReadByte(object + 0x3cu) == 0u ? 1u : 0u;
    runtime.WriteByte(object + 0x3cu, 0u);
    return 0u;
}

std::uint8_t StartTask::SpawnFollowup(
    const std::uint32_t object, IStartTaskSpawnRuntime& runtime) {
    const auto context = runtime.ReadWord(object + 0x5cu);
    if (context == 0u) return 0u;
    const auto owner = runtime.ReadWord(object + 0x58u);
    if (owner == 0u) return 0u;
    const auto child = StartTaskFactory::Create(runtime);
    if (child != 0u)
        runtime.AddReverseTask(context, child, object + 0x0cu);
    (void)Initialize(child, owner, ReadVector(object + 0x40u, runtime),
                     ReadVector(object + 0x4cu, runtime), 0u, runtime);
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

std::uint32_t StartTaskFactory::ConstructDescriptor(
    const std::uint32_t descriptor, IStartTaskRuntime& runtime) {
    runtime.WriteWord(descriptor, kDescriptorVtable);
    return descriptor;
}

std::uint32_t StartTask::Initialize(
    const std::uint32_t object, const std::uint32_t owner,
    const MoveTaskVector& position, const MoveTaskVector& rotation,
    const std::uint32_t context, IStartTaskRuntime& runtime) {
    runtime.WriteWord(object + 0x58u, owner);
    WriteVector(object + 0x40u, position, runtime);
    WriteVector(object + 0x4cu, rotation, runtime);
    runtime.WriteByte(object + 0x3cu, 0u);
    runtime.WriteWord(object + 0x5cu, context);
    return std::bit_cast<std::uint32_t>(rotation.z);
}

std::uint32_t StartTaskFactory::Create(
    IStartTaskSpawnRuntime& runtime) {
    const auto memory = runtime.AllocateMemory(0x60u);
    if (memory == 0u) return 0u;
    try {
        return StartTask::Construct(memory, runtime);
    } catch (...) {
        runtime.FreeMemory(memory);
        throw;
    }
}

}  // namespace pal4
