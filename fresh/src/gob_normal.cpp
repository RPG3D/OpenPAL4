#include "pal4/gob_normal.h"
#include "pal4/gob_normal_snapshot.h"
#include "pal4/gob_scene_package.h"

#include <string>

namespace pal4 {

std::uint32_t GobNormal::Create(IGobNormalRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) return 0u;
    try {
        return Construct(object, runtime);
    } catch (...) {
        runtime.Free(object);
        throw;
    }
}

std::uint32_t GobNormal::Construct(const std::uint32_t object,
                                   IGobNormalConstructRuntime& runtime) {
    runtime.ConstructBase(object);
    runtime.WriteWord(object, kVtable);
    return object;
}

std::uint32_t GobNormal::ValidateScript() { return 0u; }

void GobNormal::Destruct(const std::uint32_t object,
                         IGobNormalConstructRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    runtime.DestructBase(object);
}

std::uint32_t GobNormal::InitializeSceneAndConfig(
    const std::uint32_t object, const std::uint32_t first,
    const std::string_view path, const std::uint32_t fourth,
    const std::uint32_t fifth, IGobNormalRuntime& runtime) {
    runtime.CleanupBeforeSceneLoad(object);
    SelectGobScenePackage(path, runtime);
    const auto result = runtime.InitializeBaseModel(
        object, first, path, fourth, fifth);
    runtime.CleanupScriptPackages();
    return result;
}

std::uint32_t GobNormal::CreateStateSnapshot(
    const std::uint32_t object, IGobNormalSnapshotRuntime& runtime) {
    if (runtime.ReadWord(object + 0x1a4u) == 0u) return 0u;
    auto& snapshot_runtime = runtime.SnapshotRuntime();
    const auto snapshot = GobNormalStateSnapshot::Create(snapshot_runtime);
    if (snapshot == 0u) return 0u;
    snapshot_runtime.AssignNameRecord(snapshot + 4u,
        runtime.ReadCString(runtime.ReadWord(object + 4u)));
    snapshot_runtime.WriteWord(snapshot + 0x14u,
        runtime.ReadWord(object + 0x184u));
    return snapshot;
}

std::uint32_t GobNormal::ApplyStateSnapshot(
    const std::uint32_t object, const std::uint32_t snapshot,
    IGobNormalSnapshotRuntime& runtime) {
    const auto enabled = runtime.ReadWord(object + 0x1a4u);
    if (enabled == 0u) return 0u;
    const auto validation = runtime.ValidateSnapshot(snapshot);
    if (validation != 0u) return validation;
    return runtime.SetVisibility(object,
        runtime.SnapshotRuntime().ReadWord(snapshot + 0x14u));
}

}  // namespace pal4
