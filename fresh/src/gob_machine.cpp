#include "pal4/gob_machine.h"
#include "pal4/gob_scene_package.h"

#include <array>

namespace pal4 {

std::uint32_t GobMachine::Create(IGobMachineRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) return 0u;
    try {
        return Construct(object, runtime);
    } catch (...) {
        runtime.Free(object);
        throw;
    }
}

std::uint32_t GobMachine::Construct(
    const std::uint32_t object, IGobMachineConstructRuntime& runtime) {
    runtime.ConstructBase(object);
    runtime.InitializeAnimationState(object + kStateOffset);
    runtime.InitializeTaskContainer(object + kPrimaryTasksOffset);
    runtime.InitializeTaskContainer(object + kContextTasksOffset);
    runtime.InitializeTaskArrayRecord(object + 0x2a0u);
    runtime.InitializeStringRecord(object + 0x2bcu);
    for (const auto offset : std::array<std::uint32_t, 13>{
             0x278u, 0x27cu, 0x280u, 0x284u,
             0x2a4u, 0x2a8u, 0x2acu,
             0x2b0u, 0x2b4u, 0x2b8u,
             0x2c0u, 0x2c4u, 0x2c8u})
        runtime.WriteWord(object + offset, 0u);
    runtime.WriteWord(object, kVtable);
    runtime.WriteWord(object + kStateOffset, kStateVtable);
    return object;
}

std::uint32_t GobMachine::TypeKey() { return 5u; }

void GobMachine::Destruct(
    const std::uint32_t object, IGobMachineConstructRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    runtime.WriteWord(object + kStateOffset, kStateVtable);
    runtime.ReleaseStringRecord(object + 0x2bcu);
    runtime.WriteWord(object + 0x2c0u, 0u);
    runtime.WriteWord(object + 0x2c4u, 0u);
    runtime.WriteWord(object + 0x2c8u, 0u);
    runtime.FreeTaskArray(runtime.ReadWord(object + 0x2a4u));
    runtime.WriteWord(object + 0x2a4u, 0u);
    runtime.WriteWord(object + 0x2a8u, 0u);
    runtime.WriteWord(object + 0x2acu, 0u);
    runtime.DestroyTaskContainer(object + kContextTasksOffset);
    runtime.DestroyTaskContainer(object + kPrimaryTasksOffset);
    runtime.CleanupAnimationState(object + kStateOffset);
    runtime.DestructBase(object);
}

std::uint32_t GobMachine::CleanupResources(
    const std::uint32_t object, IGobMachineCleanupRuntime& runtime) {
    runtime.CleanupAnimationData(object + kStateOffset);
    runtime.CleanupBaseResources(object);
    runtime.ClearTaskContainer(object + kPrimaryTasksOffset);
    runtime.ClearTaskContainer(object + kContextTasksOffset);
    runtime.WriteWord(object + 0x2a8u,
                      runtime.ReadWord(object + 0x2a4u));
    for (const auto offset : {0x278u, 0x27cu, 0x280u, 0x284u})
        runtime.WriteWord(object + offset, 0u);
    return 0u;
}

std::uint32_t GobMachine::UpdateRuntimeAndTasks(
    const std::uint32_t object, const float elapsed,
    IGobMachineFrameRuntime& runtime) {
    runtime.UpdateBaseClump(elapsed);
    runtime.UpdateAnimation(object + kStateOffset, elapsed);
    const auto pending = runtime.ReadWord(object + 0x278u);
    if (pending != 0u) return pending;
    const auto running = runtime.ReadWord(object + 0x27cu);
    if (running == 0u) return running;
    if (runtime.ReadByte(object + 0x264u) == 0u)
        return runtime.UpdateTaskContainer(
            object + kPrimaryTasksOffset, elapsed);
    if (runtime.ReadByte(object + 0x274u) == 0u)
        return runtime.UpdateTaskContainer(
            object + kContextTasksOffset, elapsed);

    runtime.WriteWord(object + 0x27cu, 0u);
    runtime.WriteWord(object + 0x2a8u,
                      runtime.ReadWord(object + 0x2a4u));
    if (runtime.ReadWord(object + 0x280u) != 0u)
        return runtime.TriggerTaskRun(object, {0.0f, 0.0f, 0.0f});

    const auto uses = runtime.ReadWord(object + 0x198u);
    if (uses != 0u) return uses;
    if (runtime.ReadWord(object + 0x190u) != 0u)
        runtime.ReaddActiveSceneObject(object, 1.0f);
    return runtime.RefreshMinimap();
}

std::uint32_t GobMachine::InitializeSceneAndConfig(
    const std::uint32_t object, const std::uint32_t first,
    const std::string_view path, const std::uint32_t fourth,
    const std::uint32_t fifth, IGobMachineRuntime& runtime) {
    runtime.CleanupBeforeSceneLoad(object);
    SelectGobScenePackage(path, runtime);
    const auto result = runtime.InitializeBaseModel(
        object, first, path, fourth, fifth);
    if (result == 0u) {
        runtime.CleanupScriptPackages();
        return 0u;
    }

    const auto clump = runtime.ReadWord(object + 0x14u);
    runtime.HideHookUserDataAtomics(clump);
    runtime.LoadAnimationData(object + kStateOffset,
                              clump, path, fourth);
    const auto document = runtime.ReadWord(object + 0x1c0u);
    if (document == 0u) {
        runtime.CleanupScriptPackages();
        return 0u;
    }

    const auto parameters = runtime.FindXmlChild(
        document, "PAL4-GameObject-Parameters");
    if (parameters != 0u) {
        const auto old = runtime.FindXmlChild(
            parameters, "PAL4_GameObject-machine");
        if (old != 0u) runtime.RemoveXmlChild(parameters, old);
        const auto machine = runtime.CreateXmlChild(
            parameters, "PAL4_GameObject-machine");
        for (int index = 0; index < 5; ++index)
            (void)runtime.CreateXmlChild(
                machine, "PAL4_GameObject-machine-condition");
    }
    runtime.ClearAnimationFlag(object + kStateOffset, 1u);
    runtime.SetAnimationFlag(object + kStateOffset, 0u);
    runtime.CleanupScriptPackages();
    return result;
}

}  // namespace pal4
