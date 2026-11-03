#include "pal4/camera_sequence_task.h"

#include "pal4/action_task_object.h"
#include "pal4/task_pointer_collection.h"
#include "pal4/task_text_assignment.h"
#include "pal4/task_text_lifecycle.h"

namespace pal4 {

namespace {

std::uint8_t SpawnCameraFollowup(
    const std::uint32_t object, const std::uint32_t context_offset,
    ICameraSequenceSpawnRuntime& runtime) {
    const auto context = runtime.ReadWord(object + context_offset);
    if (context == 0u) return 0u;
    const auto owner = runtime.ReadWord(object + 0x40u);
    if (owner == 0u) return 0u;
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

}  // namespace

std::uint32_t CameraSequenceTask::Construct(
    const std::uint32_t object, ICameraSequenceTaskRuntime& runtime) {
    return TaskTextLifecycle::Construct(object, 0x84874cu, runtime);
}

std::uint32_t CameraSequenceTask::Destruct(
    const std::uint32_t object, ICameraSequenceTaskRuntime& runtime) {
    return TaskTextLifecycle::Destruct(object, 0x84874cu, runtime);
}

std::uint32_t CameraSequenceTaskFactory::ConstructDescriptor(
    const std::uint32_t descriptor, ICameraSequenceTaskRuntime& runtime) {
    runtime.WriteWord(descriptor, kDescriptorVtable);
    return descriptor;
}

std::uint8_t CameraSequenceTask::Enter(
    const std::uint32_t object, ICameraSequencePlaybackRuntime& runtime) {
    if (runtime.ReadWord(object + 0x40u) == 0u) return 0u;

    auto filename = runtime.ReadWord(object + 0x48u);
    if (filename == 0u) filename = runtime.EmptyStringAddress();
    const auto scene = runtime.ReadWord(runtime.GetArenaManager() + 0x0cu);
    const auto world = runtime.ReadWord(runtime.GetArenaManager() + 0x08u);
    if (!runtime.LoadCameraFile(runtime.GetScriptCloudManager(), world,
                                scene, filename)) {
        runtime.WriteByte(object + 0x3cu, 1u);
        return 0u;
    }

    const auto active = runtime.GetActiveCamera(runtime.GetCameraSubsystem());
    runtime.CleanupCameraTasks(CameraActiveObject::TaskCollection(active));
    auto record = runtime.FirstCamera(runtime.GetScriptCloudManager());
    while (record != 0u) {
        const CameraSequenceTiming timing{
            1u, 0u,
            runtime.ReadWord(record + 0x38u) != 0u ?
                0u : runtime.ReadWord(record + 0x34u)};
        const auto container = CameraActiveObject::TaskCollection(active);
        const auto task = runtime.CreateCameraTask(
            CameraSequenceTaskFactory::kDescriptorVtable);
        if (task != 0u) runtime.AddCameraTask(container, task, timing);
        const auto property = runtime.GetInternalId(active);
        (void)CameraTaskObject::Configure(task, property, record, runtime);
        record = runtime.NextCamera(runtime.GetScriptCloudManager());
    }

    runtime.MarkCameraProcessed(CameraActiveObject::TaskCollection(active));
    const auto control = runtime.GetActorControl();
    runtime.WriteWord(object + 0x58u, runtime.ReadWord(control));
    const auto property = runtime.GetInternalId(active);
    runtime.WriteWord(object + 0x5cu,
                      runtime.ReadWord(property + 0x1a8u));
    runtime.ResetActorControl(runtime.GetActorControl(), 0u);
    runtime.WriteWord(runtime.GetInternalId(active) + 0x1a8u, 0u);
    return 1u;
}

std::uint8_t CameraSequenceTask::Update(
    const std::uint32_t object, ICameraSequencePlaybackRuntime& runtime) {
    if (runtime.ReadWord(object + 0x40u) == 0u) {
        runtime.WriteByte(object + 0x3cu, 0u);
        return 0u;
    }
    if (runtime.ReadByte(object + 0x3cu) != 0u) return 0u;

    const auto active = runtime.GetActiveCamera(runtime.GetCameraSubsystem());
    if (runtime.ReadByte(active + 0x1cu) != 0u) {
        runtime.WriteByte(object + 0x3cu, 1u);
        const auto control = runtime.GetActorControl();
        const auto previous_control = runtime.ReadWord(object + 0x58u);
        runtime.ResetActorControl(control, previous_control);
        const auto previous_camera_mode = runtime.ReadWord(object + 0x5cu);
        const auto property = runtime.GetInternalId(active);
        runtime.WriteWord(property + 0x1a8u, previous_camera_mode);
        if (runtime.ReadWord(object + 0x54u) != 0u) {
            const auto mode = runtime.ReadWord(
                runtime.GetCameraSubsystem() + 0xc0u);
            runtime.SetCameraMode(runtime.GetCameraSubsystem(), mode);
            runtime.SetLookAtFromObject(active, 1u);
        }
    }
    return 1u;
}

std::uint8_t CameraSequenceTask::SpawnFollowup(
    const std::uint32_t object, ICameraSequenceSpawnRuntime& runtime) {
    return SpawnCameraFollowup(object, 0x60u, runtime);
}

std::uint32_t CameraSequenceTask::Initialize(
    const std::uint32_t object, const std::uint32_t owner,
    const std::uint32_t filename, const std::uint32_t autoseek,
    const std::uint32_t context,
    ICameraSequenceInitializeRuntime& runtime) {
    runtime.WriteWord(object + 0x40u, owner);
    runtime.WriteByte(object + 0x3cu, 0u);
    return TaskTextAssignment::Assign(object, filename, autoseek, context,
                                      0x60u, runtime);
}

std::uint32_t CameraAutoSeekTask::Construct(
    const std::uint32_t object, ICameraSequenceTaskRuntime& runtime) {
    runtime.BaseConstruct(object);
    runtime.WriteWord(object, 0x848774u);
    runtime.WriteByte(object + 0x3cu, 0u);
    return object;
}

std::uint32_t CameraAutoSeekTask::Destruct(
    const std::uint32_t object, ICameraSequenceTaskRuntime& runtime) {
    runtime.WriteByte(object + 0x3cu, 0u);
    runtime.WriteWord(object, 0x8486fcu);
    return runtime.BaseDestruct(object);
}

std::uint32_t CameraAutoSeekTask::Delete(
    const std::uint32_t object, const std::uint8_t flags,
    ICameraSequenceTaskRuntime& runtime) {
    (void)Destruct(object, runtime);
    if ((flags & 1u) != 0u) runtime.FreeMemory(object);
    return object;
}

std::uint8_t CameraAutoSeekTask::Enter(
    const std::uint32_t object, ICameraSequencePlaybackRuntime& runtime) {
    if (runtime.ReadWord(object + 0x40u) == 0u) return 0u;
    const auto active = runtime.GetActiveCamera(runtime.GetCameraSubsystem());
    const auto mode = runtime.ReadWord(runtime.GetCameraSubsystem() + 0xc0u);
    runtime.SetCameraMode(runtime.GetCameraSubsystem(), mode);
    runtime.SetLookAtFromObject(active, 1u);
    runtime.WriteByte(object + 0x3cu, 1u);
    return 1u;
}

std::uint8_t CameraAutoSeekTask::SpawnFollowup(
    const std::uint32_t object, ICameraSequenceSpawnRuntime& runtime) {
    return SpawnCameraFollowup(object, 0x44u, runtime);
}

std::uint32_t GomTaskOwnerContext::Initialize(
    const std::uint32_t object, const std::uint32_t owner,
    const std::uint32_t context, ICameraSequenceTaskRuntime& runtime) {
    runtime.WriteWord(object + 0x40u, owner);
    runtime.WriteByte(object + 0x3cu, 0u);
    runtime.WriteWord(object + 0x44u, context);
    return owner;
}

std::uint32_t GomTaskOwnerContext::InitializePlain(
    const std::uint32_t object, const std::uint32_t owner,
    const std::uint32_t context, ICameraSequenceTaskRuntime& runtime) {
    runtime.WriteWord(object + 0x3cu, owner);
    runtime.WriteWord(object + 0x40u, context);
    return owner;
}

std::uint32_t CameraQuakeTask::Construct(
    const std::uint32_t object, ICameraSequenceTaskRuntime& runtime) {
    runtime.BaseConstruct(object);
    runtime.WriteWord(object, 0x848798u);
    runtime.WriteWord(object + 0x3cu, 0u);
    runtime.WriteWord(object + 0x48u, 0u);
    runtime.WriteWord(object + 0x4cu, 0u);
    return object;
}

std::uint32_t CameraQuakeTask::Destruct(
    const std::uint32_t object, ICameraSequenceTaskRuntime& runtime) {
    runtime.WriteWord(object, 0x8486fcu);
    runtime.WriteWord(object + 0x3cu, 0u);
    runtime.WriteWord(object + 0x48u, 0u);
    runtime.WriteWord(object + 0x4cu, 0u);
    return runtime.BaseDestruct(object);
}

std::uint8_t CameraQuakeTask::Enter(
    const std::uint32_t object, ICameraQuakeRuntime& runtime) {
    if (runtime.ReadWord(object + 0x48u) == 0u) return 0u;
    const auto subsystem = runtime.GetCameraSubsystem();
    const auto frequency = runtime.ReadWord(object + 0x44u);
    const auto amplitude = runtime.ReadWord(object + 0x40u);
    const auto time = runtime.ReadWord(object + 0x3cu);
    const auto active = runtime.GetActiveCameraInternalId(subsystem);
    runtime.InitializeCameraParameters(active, time, amplitude, frequency);
    runtime.SetCameraActive(
        runtime.GetActiveCameraInternalId(runtime.GetCameraSubsystem()), 1u);
    return 1u;
}

bool CameraQuakeTask::ValidateOwner(
    const std::uint32_t object, ICameraSequenceTaskRuntime& runtime) {
    return runtime.ReadWord(object + 0x48u) != 0u;
}

std::uint8_t CameraQuakeTask::SpawnFollowup(
    const std::uint32_t object, ICameraQuakeSpawnRuntime& runtime) {
    const auto context = runtime.ReadWord(object + 0x4cu);
    if (context == 0u) return 0u;
    const auto owner = runtime.ReadWord(object + 0x48u);
    if (owner == 0u) return 0u;

    const auto quake = CameraQuakeTaskFactory::Create(runtime);
    if (quake != 0u)
        runtime.AddQuakeTask(context, quake, object + 0x0cu);
    const auto time = runtime.ReadWord(object + 0x3cu);
    const auto amplitude = runtime.ReadWord(object + 0x40u);
    const auto frequency = runtime.ReadWord(object + 0x44u);
    (void)CameraQuakeTask::Initialize(quake, owner, time, amplitude,
                                     frequency, 0u, runtime);
    runtime.InsertOwnerTask(owner + 0x2a0u, quake);

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

std::uint32_t CameraQuakeTaskFactory::ConstructDescriptor(
    const std::uint32_t descriptor, ICameraSequenceTaskRuntime& runtime) {
    runtime.WriteWord(descriptor, kDescriptorVtable);
    return descriptor;
}

std::uint32_t CameraQuakeTask::Initialize(
    const std::uint32_t object, const std::uint32_t owner,
    const std::uint32_t time, const std::uint32_t amplitude,
    const std::uint32_t frequency, const std::uint32_t context,
    ICameraSequenceTaskRuntime& runtime) {
    runtime.WriteWord(object + 0x48u, owner);
    runtime.WriteWord(object + 0x3cu, time);
    runtime.WriteWord(object + 0x40u, amplitude);
    runtime.WriteWord(object + 0x44u, frequency);
    runtime.WriteWord(object + 0x4cu, context);
    return context;
}

std::uint32_t CameraQuakeTaskFactory::Create(
    ICameraSequenceSpawnRuntime& runtime) {
    const auto memory = runtime.AllocateMemory(0x50u);
    if (memory == 0u) return 0u;
    try {
        return CameraQuakeTask::Construct(memory, runtime);
    } catch (...) {
        runtime.FreeMemory(memory);
        throw;
    }
}

std::uint32_t CameraQuakeTerminateTask::Construct(
    const std::uint32_t object, ICameraSequenceTaskRuntime& runtime) {
    runtime.BaseConstruct(object);
    runtime.WriteWord(object, 0x8487c0u);
    runtime.WriteWord(object + 0x3cu, 0u);
    runtime.WriteWord(object + 0x40u, 0u);
    return object;
}

std::uint32_t CameraQuakeTerminateTask::Destruct(
    const std::uint32_t object, ICameraSequenceTaskRuntime& runtime) {
    runtime.WriteWord(object, 0x8486fcu);
    runtime.WriteWord(object + 0x3cu, 0u);
    runtime.WriteWord(object + 0x40u, 0u);
    return runtime.BaseDestruct(object);
}

std::uint32_t CameraQuakeTerminateTask::Delete(
    const std::uint32_t object, const std::uint8_t flags,
    ICameraSequenceTaskRuntime& runtime) {
    (void)Destruct(object, runtime);
    if ((flags & 1u) != 0u) runtime.FreeMemory(object);
    return object;
}

std::uint8_t CameraQuakeTerminateTask::Enter(ICameraQuakeRuntime& runtime) {
    const auto active = runtime.GetActiveCameraInternalId(
        runtime.GetCameraSubsystem());
    runtime.SetCameraActive(active, 0u);
    return 1u;
}

bool CameraQuakeTerminateTask::ValidateOwner(
    const std::uint32_t object, ICameraSequenceTaskRuntime& runtime) {
    return runtime.ReadWord(object + 0x3cu) != 0u;
}

std::uint8_t CameraQuakeTerminateTask::SpawnFollowup(
    const std::uint32_t object,
    ICameraQuakeTerminateSpawnRuntime& runtime) {
    const auto context = runtime.ReadWord(object + 0x40u);
    if (context == 0u) return 0u;
    const auto owner = runtime.ReadWord(object + 0x3cu);
    if (owner == 0u) return 0u;

    const auto child = CameraQuakeTerminateTaskFactory::Create(runtime);
    if (child != 0u)
        runtime.AddTerminateTask(context, child, object + 0x0cu);
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

std::uint32_t CameraQuakeTerminateTaskFactory::ConstructDescriptor(
    const std::uint32_t descriptor, ICameraSequenceTaskRuntime& runtime) {
    runtime.WriteWord(descriptor, kDescriptorVtable);
    return descriptor;
}

std::uint32_t CameraQuakeTerminateTaskFactory::Create(
    ICameraSequenceSpawnRuntime& runtime) {
    const auto memory = runtime.AllocateMemory(0x44u);
    if (memory == 0u) return 0u;
    try {
        return CameraQuakeTerminateTask::Construct(memory, runtime);
    } catch (...) {
        runtime.FreeMemory(memory);
        throw;
    }
}

}  // namespace pal4
