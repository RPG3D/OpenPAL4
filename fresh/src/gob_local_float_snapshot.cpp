#include "pal4/gob_local_float_snapshot.h"

#include <array>

namespace pal4 {

namespace {

constexpr std::array<std::string_view, 6> kFloatKeys{
    "valPosX", "valPosY", "valPosZ",
    "valRotX", "valRotY", "valRotZ"};

}  // namespace

std::uint32_t GobLocalFloatStateSnapshot::Create(
    IGobLocalFloatSnapshotRuntime& runtime) {
    const auto snapshot = runtime.Allocate(kSize);
    if (snapshot == 0u) return 0u;
    try {
        return Construct(snapshot, runtime);
    } catch (...) {
        runtime.Free(snapshot);
        throw;
    }
}

std::uint32_t GobLocalFloatStateSnapshot::Construct(
    const std::uint32_t snapshot,
    IGobLocalFloatSnapshotRuntime& runtime) {
    runtime.InitializeNameRecord(snapshot + 4u);
    runtime.WriteWord(snapshot + 8u, 0u);
    runtime.WriteWord(snapshot + 0xcu, 0u);
    runtime.WriteWord(snapshot + 0x10u, 0u);
    runtime.WriteWord(snapshot, kVtable);
    return snapshot;
}

std::uint32_t GobLocalFloatStateSnapshot::TypeKey() { return 2u; }

std::uint32_t GobLocalFloatStateSnapshot::Destruct(
    const std::uint32_t snapshot,
    IGobLocalFloatSnapshotRuntime& runtime) {
    runtime.WriteWord(snapshot, kVtable);
    const auto result = runtime.ReleaseNameRecord(snapshot + 4u);
    runtime.WriteWord(snapshot + 8u, 0u);
    runtime.WriteWord(snapshot + 0xcu, 0u);
    runtime.WriteWord(snapshot + 0x10u, 0u);
    runtime.WriteWord(snapshot, kBaseVtable);
    return result;
}

std::uint32_t GobLocalFloatStateSnapshot::WriteConfig(
    const std::uint32_t snapshot, const std::uint32_t stream,
    IGobLocalFloatSnapshotRuntime& runtime) {
    const auto document = runtime.CreateXmlDocument("soGOBLocalFloat");
    runtime.WriteXmlString(document, "valName",
                           runtime.ReadNameRecord(snapshot + 4u));
    runtime.WriteXmlInt(document, "valVisible",
                        static_cast<std::int32_t>(
                            runtime.ReadWord(snapshot + 0x14u)));
    runtime.WriteXmlInt(document, "researchNum",
                        static_cast<std::int32_t>(
                            runtime.ReadWord(snapshot + 0x30u)));
    for (std::uint32_t i = 0u; i < kFloatKeys.size(); ++i)
        runtime.WriteXmlFloat(document, kFloatKeys[i],
                              runtime.ReadFloat(snapshot + 0x18u + i * 4u));
    runtime.FlushXml(document, stream);
    return runtime.DestroyXmlDocument(document);
}

std::uint32_t GobLocalFloatStateSnapshot::ReadConfig(
    const std::uint32_t snapshot, const std::uint32_t stream,
    IGobLocalFloatSnapshotRuntime& runtime) {
    const auto document = runtime.CreateXmlDocument("soGOBLocalFloat");
    if (runtime.ReadXml(document, stream)) {
        runtime.AssignNameRecord(
            snapshot + 4u,
            runtime.ReadXmlString(document, "valName", ""));
        runtime.WriteWord(snapshot + 0x14u,
                          static_cast<std::uint32_t>(
                              runtime.ReadXmlInt(document,
                                                 "valVisible", 0)));
        runtime.WriteWord(snapshot + 0x30u,
                          static_cast<std::uint32_t>(
                              runtime.ReadXmlInt(document,
                                                 "researchNum", 0)));
        for (std::uint32_t i = 0u; i < kFloatKeys.size(); ++i)
            runtime.WriteFloat(
                snapshot + 0x18u + i * 4u,
                runtime.ReadXmlFloat(document, kFloatKeys[i], 0.0f));
    }
    return runtime.DestroyXmlDocument(document);
}

}  // namespace pal4
