#include "pal4/gob_marker_snapshot.h"

namespace pal4 {

std::uint32_t GobMarkerStateSnapshot::Create(
    IGobSnapshotRuntime& runtime) {
    const auto snapshot = runtime.Allocate(kSize);
    if (snapshot == 0u) return 0u;
    try {
        return Construct(snapshot, runtime);
    } catch (...) {
        runtime.Free(snapshot);
        throw;
    }
}

std::uint32_t GobMarkerStateSnapshot::Construct(
    const std::uint32_t snapshot, IGobSnapshotRuntime& runtime) {
    runtime.InitializeNameRecord(snapshot + 4u);
    runtime.WriteWord(snapshot + 8u, 0u);
    runtime.WriteWord(snapshot + 0xcu, 0u);
    runtime.WriteWord(snapshot + 0x10u, 0u);
    runtime.WriteWord(snapshot, kVtable);
    return snapshot;
}

std::uint32_t GobMarkerStateSnapshot::Destruct(
    const std::uint32_t snapshot, IGobSnapshotRuntime& runtime) {
    runtime.WriteWord(snapshot, kVtable);
    const auto result = runtime.ReleaseNameRecord(snapshot + 4u);
    runtime.WriteWord(snapshot + 8u, 0u);
    runtime.WriteWord(snapshot + 0xcu, 0u);
    runtime.WriteWord(snapshot + 0x10u, 0u);
    runtime.WriteWord(snapshot, kBaseVtable);
    return result;
}

std::uint32_t GobMarkerStateSnapshot::WriteConfig(
    const std::uint32_t snapshot, const std::uint32_t stream,
    IGobSnapshotRuntime& runtime) {
    const auto document = runtime.CreateXmlDocument("soGOBMarker");
    runtime.WriteXmlString(document, "valName",
                           runtime.ReadNameRecord(snapshot + 4u));
    runtime.FlushXml(document, stream);
    return runtime.DestroyXmlDocument(document);
}

std::uint32_t GobMarkerStateSnapshot::ReadConfig(
    const std::uint32_t snapshot, const std::uint32_t stream,
    IGobSnapshotRuntime& runtime) {
    const auto document = runtime.CreateXmlDocument("soGOBMarker");
    if (runtime.ReadXml(document, stream))
        runtime.AssignNameRecord(snapshot + 4u,
            runtime.ReadXmlString(document, "valName", ""));
    return runtime.DestroyXmlDocument(document);
}

}  // namespace pal4
