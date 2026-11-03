#include "pal4/gob_marker.h"
#include "pal4/gob_marker_snapshot.h"
#include "pal4/gob_scene_package.h"

namespace pal4 {

std::uint32_t GobMarker::Create(IGobMarkerRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) return 0u;
    try {
        return Construct(object, runtime);
    } catch (...) {
        runtime.Free(object);
        throw;
    }
}

std::uint32_t GobMarker::Construct(
    const std::uint32_t object, IGobMarkerConstructRuntime& runtime) {
    runtime.ConstructBase(object);
    runtime.WriteWord(object, kVtable);
    runtime.WriteWord(object + 0x170u, 0u);
    return object;
}

std::uint32_t GobMarker::TypeKey() { return 9u; }

void GobMarker::Destruct(
    const std::uint32_t object, IGobMarkerConstructRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    runtime.DestructBase(object);
}

std::uint32_t GobMarker::InitializeSceneAndConfig(
    const std::uint32_t object, const std::uint32_t first,
    const std::string_view path, const std::uint32_t fourth,
    const std::uint32_t fifth, IGobMarkerRuntime& runtime) {
    runtime.CleanupBeforeSceneLoad(object);
    SelectGobScenePackage(path, runtime);
    const auto result = runtime.InitializeBaseModel(
        object, first, path, fourth, fifth);
    runtime.CleanupScriptPackages();
    return result;
}

std::uint32_t GobMarker::CreateStateSnapshot(
    const std::uint32_t object, IGobMarkerSnapshotCaptureRuntime& runtime) {
    if (runtime.ReadWord(object + 0x1a4u) == 0u) return 0u;
    auto& snapshot_runtime = runtime.SnapshotRuntime();
    const auto snapshot = GobMarkerStateSnapshot::Create(snapshot_runtime);
    if (snapshot == 0u) return 0u;
    snapshot_runtime.AssignNameRecord(
        snapshot + 4u, runtime.ReadCString(runtime.ReadWord(object + 4u)));
    return snapshot;
}

}  // namespace pal4
