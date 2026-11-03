#include "pal4/gob_local_floor.h"
#include "pal4/gob_scene_package.h"

namespace pal4 {

std::uint32_t GobLocalFloor::Create(IGobLocalFloorRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) return 0u;
    try {
        return Construct(object, runtime);
    } catch (...) {
        runtime.Free(object);
        throw;
    }
}

std::uint32_t GobLocalFloor::Construct(
    const std::uint32_t object, IGobLocalFloorRuntime& runtime) {
    runtime.ConstructBase(object);
    runtime.WriteWord(object + kHelperOffset, kHelperBaseVtable);
    runtime.WriteWord(object, kVtable);
    runtime.WriteWord(object + kHelperOffset, kHelperVtable);
    return object;
}

std::uint32_t GobLocalFloor::TypeKey() { return 1u; }

double GobLocalFloor::InteractionThreshold() { return 100.0; }

void GobLocalFloor::Destruct(
    const std::uint32_t object, IGobLocalFloorRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    runtime.WriteWord(object + kHelperOffset, kHelperVtable);
    runtime.WriteWord(object + kHelperOffset, kHelperBaseVtable);
    runtime.DestructBase(object);
}

std::uint32_t GobLocalFloor::CleanupResources(
    const std::uint32_t object, const std::uint32_t context,
    IGobInheritedCleanupRuntime& runtime) {
    return runtime.CleanupBase(object, context);
}

std::uint32_t GobLocalFloor::RenderIfVisible(
    const std::uint32_t object, const std::uint32_t context,
    IGobLocalFloorRuntime& runtime) {
    return runtime.RenderBaseIfVisible(object, context);
}

std::uint32_t GobLocalFloor::UpdateClumpIfVisible(
    const float elapsed, IGobLocalFloorRuntime& runtime) {
    return runtime.UpdateBaseClumpIfVisible(elapsed);
}

std::uint32_t GobLocalFloor::InitializeSceneAndConfig(
    const std::uint32_t object, const std::uint32_t first,
    const std::string_view path, const std::uint32_t fourth,
    const std::uint32_t fifth, IGobLocalFloorRuntime& runtime) {
    runtime.CleanupBeforeSceneLoad(object);
    SelectGobScenePackage(path, runtime);
    const auto result = runtime.InitializeBaseModel(
        object, first, path, fourth, fifth);
    runtime.CleanupScriptPackages();
    return result;
}

std::uint32_t GobLocalFloor::AttachMovementAdapter(
    const std::uint32_t object, IGobLocalFloorRuntime& runtime) {
    return runtime.AttachMovementHelper(
        object == 0u ? 0u : object + kHelperOffset);
}

std::uint32_t GobLocalFloor::DetachMovementAdapter(
    const std::uint32_t object, IGobLocalFloorRuntime& runtime) {
    return runtime.DetachMovementHelper(
        object == 0u ? 0u : object + kHelperOffset);
}

std::uint32_t GobLocalFloor::HelperClump(
    const std::uint32_t helper, IGobLocalFloorRuntime& runtime) {
    return runtime.ReadWord(helper - kHelperOffset + 0x14u);
}

std::uint32_t GobLocalFloor::CreateStateSnapshot(
    const std::uint32_t object, IGobLocalFloorRuntime& runtime) {
    if (runtime.ReadWord(object + 0x1a4u) == 0u) return 0u;
    const auto snapshot = runtime.Allocate(0x18u);
    if (snapshot == 0u) return 0u;
    try {
        runtime.ConstructSnapshot(snapshot);
        runtime.AssignSnapshotName(snapshot,
            runtime.ReadCString(runtime.ReadWord(object + 4u)));
        runtime.WriteWord(snapshot + 0x14u,
                          runtime.ReadWord(object + 0x184u));
    } catch (...) {
        runtime.Free(snapshot);
        throw;
    }
    return snapshot;
}

std::uint32_t GobLocalFloor::ApplyStateSnapshot(
    const std::uint32_t object, const std::uint32_t snapshot,
    IGobLocalFloorRuntime& runtime) {
    const auto enabled = runtime.ReadWord(object + 0x1a4u);
    if (enabled == 0u) return enabled;
    const auto type = runtime.SnapshotType(snapshot);
    if (type != TypeKey()) return type;
    return runtime.SetBaseVisible(
        object, runtime.ReadWord(snapshot + 0x14u));
}

}  // namespace pal4
