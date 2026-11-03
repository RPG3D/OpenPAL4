#include "pal4/gob_sound.h"
#include "pal4/gob_sound_snapshot.h"

namespace pal4 {

std::uint8_t GobSound::SetVisible(
    const std::uint32_t object, const std::uint32_t visible,
    IGobSoundRuntime& runtime) {
    return runtime.SetBaseVisible(object, visible);
}

std::uint32_t GobSound::CreateStateSnapshot(
    const std::uint32_t object, IGobSoundSnapshotCaptureRuntime& runtime) {
    if (runtime.ReadWord(object + 0x1a4u) == 0u) return 0u;
    auto& snapshot_runtime = runtime.SnapshotRuntime();
    const auto snapshot = GobSoundStateSnapshot::Create(snapshot_runtime);
    if (snapshot == 0u) return 0u;
    snapshot_runtime.AssignNameRecord(
        snapshot + 4u, runtime.ReadCString(runtime.ReadWord(object + 4u)));
    return snapshot;
}

std::uint32_t GobSoundStateSnapshot::Create(IGobSnapshotRuntime& runtime) {
    const auto snapshot = runtime.Allocate(kSize);
    if (snapshot == 0u) return 0u;
    try {
        return Construct(snapshot, runtime);
    } catch (...) {
        runtime.Free(snapshot);
        throw;
    }
}

std::uint32_t GobSoundStateSnapshot::Construct(
    const std::uint32_t snapshot, IGobSnapshotRuntime& runtime) {
    runtime.InitializeNameRecord(snapshot + 4u);
    runtime.WriteWord(snapshot + 8u, 0u);
    runtime.WriteWord(snapshot + 0xcu, 0u);
    runtime.WriteWord(snapshot + 0x10u, 0u);
    runtime.WriteWord(snapshot, kVtable);
    return snapshot;
}

std::uint32_t GobSoundStateSnapshot::Destruct(
    const std::uint32_t snapshot, IGobSnapshotRuntime& runtime) {
    runtime.WriteWord(snapshot, kVtable);
    const auto result = runtime.ReleaseNameRecord(snapshot + 4u);
    runtime.WriteWord(snapshot + 8u, 0u);
    runtime.WriteWord(snapshot + 0xcu, 0u);
    runtime.WriteWord(snapshot + 0x10u, 0u);
    runtime.WriteWord(snapshot, kBaseVtable);
    return result;
}

std::uint32_t GobSoundStateSnapshot::WriteConfig(
    const std::uint32_t snapshot, const std::uint32_t stream,
    IGobSnapshotRuntime& runtime) {
    const auto document = runtime.CreateXmlDocument("soGOBSound");
    runtime.WriteXmlString(document, "valName",
                           runtime.ReadNameRecord(snapshot + 4u));
    runtime.FlushXml(document, stream);
    return runtime.DestroyXmlDocument(document);
}

std::uint32_t GobSoundStateSnapshot::ReadConfig(
    const std::uint32_t snapshot, const std::uint32_t stream,
    IGobSnapshotRuntime& runtime) {
    const auto document = runtime.CreateXmlDocument("soGOBSound");
    if (runtime.ReadXml(document, stream))
        runtime.AssignNameRecord(snapshot + 4u,
            runtime.ReadXmlString(document, "valName", ""));
    return runtime.DestroyXmlDocument(document);
}

}  // namespace pal4
