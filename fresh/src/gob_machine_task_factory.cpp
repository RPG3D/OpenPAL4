#include "pal4/gob_machine_task_factory.h"

namespace pal4 {
namespace {

std::uint32_t Construct(const std::uint32_t factory,
                        const std::uint32_t vtable,
                        IGobMachineTaskFactoryRuntime& runtime) {
    runtime.WriteWord(factory, vtable);
    return factory;
}

std::uint32_t Create(const std::uint32_t size,
                     const GobMachineTaskKind kind,
                     IGobMachineTaskCreationRuntime& runtime) {
    const auto task = runtime.Allocate(size);
    if (task == 0u) return 0u;
    try {
        return runtime.ConstructTask(kind, task);
    } catch (...) {
        runtime.Free(task);
        throw;
    }
}

}  // namespace

std::uint32_t GobMachineTaskFactory::CreateScript(
    IGobMachineTaskCreationRuntime& runtime) {
    return Create(0x5cu, GobMachineTaskKind::Script, runtime);
}

std::uint32_t GobMachineTaskFactory::CreateRollback(
    IGobMachineTaskCreationRuntime& runtime) {
    return Create(0x44u, GobMachineTaskKind::Rollback, runtime);
}

std::uint32_t GobMachineTaskFactory::CreateGetItem(
    IGobMachineTaskCreationRuntime& runtime) {
    return Create(0x54u, GobMachineTaskKind::GetItem, runtime);
}

std::uint32_t GobMachineTaskFactory::CreateCamera(
    IGobMachineTaskCreationRuntime& runtime) {
    return Create(0x64u, GobMachineTaskKind::Camera, runtime);
}

std::uint32_t GobMachineTaskFactory::CreateCameraAutoSeek(
    IGobMachineTaskCreationRuntime& runtime) {
    return Create(0x48u, GobMachineTaskKind::CameraAutoSeek, runtime);
}

std::uint32_t GobMachineTaskFactory::CreateLockControl(
    IGobMachineTaskCreationRuntime& runtime) {
    return Create(0x48u, GobMachineTaskKind::LockControl, runtime);
}

std::uint32_t GobMachineTaskFactory::CreateUnlockControl(
    IGobMachineTaskCreationRuntime& runtime) {
    return Create(0x48u, GobMachineTaskKind::UnlockControl, runtime);
}

std::uint32_t GobMachineTaskFactory::CreateActiveObject(
    IGobMachineTaskCreationRuntime& runtime) {
    return Create(0x5cu, GobMachineTaskKind::ActiveObject, runtime);
}

std::uint32_t GobMachineTaskFactory::CreateTouchOther(
    IGobMachineTaskCreationRuntime& runtime) {
    return Create(0x5cu, GobMachineTaskKind::TouchOther, runtime);
}

std::uint32_t GobMachineTaskFactory::ConstructScript(
    const std::uint32_t factory, IGobMachineTaskFactoryRuntime& runtime) {
    return Construct(factory, 0x849018u, runtime);
}

std::uint32_t GobMachineTaskFactory::ConstructRollback(
    const std::uint32_t factory, IGobMachineTaskFactoryRuntime& runtime) {
    return Construct(factory, 0x849014u, runtime);
}

std::uint32_t GobMachineTaskFactory::ConstructGetItem(
    const std::uint32_t factory, IGobMachineTaskFactoryRuntime& runtime) {
    return Construct(factory, 0x849010u, runtime);
}

std::uint32_t GobMachineTaskFactory::ConstructCamera(
    const std::uint32_t factory, IGobMachineTaskFactoryRuntime& runtime) {
    return Construct(factory, 0x84900cu, runtime);
}

std::uint32_t GobMachineTaskFactory::ConstructCameraAutoSeek(
    const std::uint32_t factory, IGobMachineTaskFactoryRuntime& runtime) {
    return Construct(factory, 0x849008u, runtime);
}

std::uint32_t GobMachineTaskFactory::ConstructLockControl(
    const std::uint32_t factory, IGobMachineTaskFactoryRuntime& runtime) {
    return Construct(factory, 0x849004u, runtime);
}

std::uint32_t GobMachineTaskFactory::ConstructUnlockControl(
    const std::uint32_t factory, IGobMachineTaskFactoryRuntime& runtime) {
    return Construct(factory, 0x849000u, runtime);
}

std::uint32_t GobMachineTaskFactory::ConstructActiveObject(
    const std::uint32_t factory, IGobMachineTaskFactoryRuntime& runtime) {
    return Construct(factory, 0x848ffcu, runtime);
}

std::uint32_t GobMachineTaskFactory::ConstructTouchOther(
    const std::uint32_t factory, IGobMachineTaskFactoryRuntime& runtime) {
    return Construct(factory, 0x848ff8u, runtime);
}

}  // namespace pal4
