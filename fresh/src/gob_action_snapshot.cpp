#include "pal4/gob_action_snapshot.h"

namespace pal4 {

std::uint32_t GobActionStateSnapshot::Create(
    IGobActionSnapshotRuntime& runtime) {
    const auto snapshot = runtime.Allocate(kSize);
    if (snapshot == 0u) return 0u;
    try {
        return Construct(snapshot, runtime);
    } catch (...) {
        runtime.Free(snapshot);
        throw;
    }
}

std::uint32_t GobActionStateSnapshot::Construct(
    const std::uint32_t snapshot,
    IGobActionSnapshotRuntime& runtime) {
    runtime.InitializeNameRecord(snapshot + 4u);
    runtime.WriteWord(snapshot + 8u, 0u);
    runtime.WriteWord(snapshot + 0xcu, 0u);
    runtime.WriteWord(snapshot + 0x10u, 0u);
    runtime.WriteWord(snapshot, kVtable);
    return snapshot;
}

std::uint32_t GobActionStateSnapshot::TypeKey() { return 6u; }

std::uint32_t GobActionStateSnapshot::Destruct(
    const std::uint32_t snapshot,
    IGobActionSnapshotRuntime& runtime) {
    runtime.WriteWord(snapshot, kVtable);
    const auto result = runtime.ReleaseNameRecord(snapshot + 4u);
    runtime.WriteWord(snapshot + 8u, 0u);
    runtime.WriteWord(snapshot + 0xcu, 0u);
    runtime.WriteWord(snapshot + 0x10u, 0u);
    runtime.WriteWord(snapshot, kBaseVtable);
    return result;
}

std::uint32_t GobActionStateSnapshot::WriteConfig(
    const std::uint32_t snapshot, const std::uint32_t stream,
    IGobActionSnapshotRuntime& runtime) {
    const auto document = runtime.CreateXmlDocument("soGOBAction");
    runtime.WriteXmlString(document, "valName",
                           runtime.ReadNameRecord(snapshot + 4u));
    runtime.WriteXmlInt(document, "valVisible",
                        static_cast<std::int32_t>(
                            runtime.ReadWord(snapshot + 0x14u)));
    runtime.WriteXmlInt(document, "playTimes",
                        static_cast<std::int32_t>(
                            runtime.ReadWord(snapshot + 0x18u)));
    runtime.FlushXml(document, stream);
    return runtime.DestroyXmlDocument(document);
}

std::uint32_t GobActionStateSnapshot::ReadConfig(
    const std::uint32_t snapshot, const std::uint32_t stream,
    IGobActionSnapshotRuntime& runtime) {
    const auto document = runtime.CreateXmlDocument("soGOBAction");
    if (runtime.ReadXml(document, stream)) {
        runtime.AssignNameRecord(
            snapshot + 4u,
            runtime.ReadXmlString(document, "valName", ""));
        runtime.WriteWord(snapshot + 0x14u,
                          static_cast<std::uint32_t>(
                              runtime.ReadXmlInt(document, "valVisible", 0)));
        runtime.WriteWord(snapshot + 0x18u,
                          static_cast<std::uint32_t>(
                              runtime.ReadXmlInt(document, "playTimes", 0)));
    }
    return runtime.DestroyXmlDocument(document);
}

}  // namespace pal4
