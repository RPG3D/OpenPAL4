#include "pal4/gob_machine.h"
#include "pal4/gob_machine_snapshot.h"

#include <array>

namespace pal4 {

std::uint32_t GobMachineStateSnapshot::Create(
    IGobMachineSnapshotRuntime& runtime) {
    const auto snapshot = runtime.Allocate(kSize);
    if (snapshot == 0u) return 0u;
    try {
        return Construct(snapshot, runtime);
    } catch (...) {
        runtime.Free(snapshot);
        throw;
    }
}

std::uint32_t GobMachineStateSnapshot::Construct(
    const std::uint32_t snapshot, IGobMachineSnapshotRuntime& runtime) {
    runtime.InitializeStringRecord(snapshot + 4u);
    for (const auto offset : {8u, 12u, 16u})
        runtime.WriteWord(snapshot + offset, 0u);
    runtime.InitializeStringRecord(snapshot + 0x40u);
    for (const auto offset : {0x44u, 0x48u, 0x4cu})
        runtime.WriteWord(snapshot + offset, 0u);
    runtime.WriteWord(snapshot, kVtable);
    return snapshot;
}

std::uint32_t GobMachineStateSnapshot::Destruct(
    const std::uint32_t snapshot, IGobMachineSnapshotRuntime& runtime) {
    runtime.WriteWord(snapshot, kVtable);
    (void)runtime.ReleaseStringRecord(snapshot + 0x40u);
    for (const auto offset : {0x44u, 0x48u, 0x4cu})
        runtime.WriteWord(snapshot + offset, 0u);
    const auto result = runtime.ReleaseStringRecord(snapshot + 4u);
    for (const auto offset : {8u, 12u, 16u})
        runtime.WriteWord(snapshot + offset, 0u);
    runtime.WriteWord(snapshot, kBaseVtable);
    return result;
}

namespace {

struct SnapshotField {
    std::string_view key;
    std::uint32_t offset;
};

constexpr std::array<SnapshotField, 6> kFloatFields{{
    {"valPosX", 0x1cu}, {"valPosY", 0x20u}, {"valPosZ", 0x24u},
    {"valRotX", 0x28u}, {"valRotY", 0x2cu}, {"valRotZ", 0x30u}}};
constexpr std::array<SnapshotField, 5> kIntegerFields{{
    {"valVisible", 0x14u}, {"researchNum", 0x18u},
    {"valHasTouched", 0x34u}, {"valNeedSetAction", 0x38u},
    {"valHasDoAction", 0x3cu}}};

}  // namespace

std::uint32_t GobMachineStateSnapshot::WriteConfig(
    const std::uint32_t snapshot, const std::uint32_t stream,
    IGobMachineSnapshotRuntime& runtime) {
    const auto document = runtime.CreateXmlDocument("soGOBMachine");
    runtime.WriteXmlString(document, "valName",
                           runtime.ReadStringRecord(snapshot + 4u));
    for (const auto& field : kIntegerFields) {
        if (field.offset == 0x34u) break;
        runtime.WriteXmlInt(document, field.key,
            static_cast<std::int32_t>(runtime.ReadWord(snapshot + field.offset)));
    }
    for (const auto& field : kFloatFields)
        runtime.WriteXmlFloat(document, field.key,
                              runtime.ReadFloat(snapshot + field.offset));
    for (const auto& field : kIntegerFields) {
        if (field.offset < 0x34u) continue;
        runtime.WriteXmlInt(document, field.key,
            static_cast<std::int32_t>(runtime.ReadWord(snapshot + field.offset)));
    }
    runtime.WriteXmlString(document, "valActionName",
                           runtime.ReadStringRecord(snapshot + 0x40u));
    runtime.FlushXml(document, stream);
    return runtime.DestroyXmlDocument(document);
}

std::uint32_t GobMachineStateSnapshot::ReadConfig(
    const std::uint32_t snapshot, const std::uint32_t stream,
    IGobMachineSnapshotRuntime& runtime) {
    const auto document = runtime.CreateXmlDocument("soGOBMachine");
    if (runtime.ReadXml(document, stream)) {
        runtime.AssignStringRecord(snapshot + 4u,
            runtime.ReadXmlString(document, "valName", ""));
        for (const auto& field : kIntegerFields) {
            if (field.offset == 0x34u) break;
            runtime.WriteWord(snapshot + field.offset,
                static_cast<std::uint32_t>(runtime.ReadXmlInt(
                    document, field.key, 0)));
        }
        for (const auto& field : kFloatFields)
            runtime.WriteFloat(snapshot + field.offset,
                runtime.ReadXmlFloat(document, field.key, 0.0f));
        for (const auto& field : kIntegerFields) {
            if (field.offset < 0x34u) continue;
            runtime.WriteWord(snapshot + field.offset,
                static_cast<std::uint32_t>(runtime.ReadXmlInt(
                    document, field.key, 0)));
        }
        runtime.AssignStringRecord(snapshot + 0x40u,
            runtime.ReadXmlString(document, "valActionName", ""));
    }
    return runtime.DestroyXmlDocument(document);
}

std::uint32_t GobMachine::CreateStateSnapshot(
    const std::uint32_t object, IGobMachineSnapshotRuntime& runtime) {
    if (runtime.ReadWord(object + 0x1a4u) == 0u) return 0u;
    const auto snapshot = GobMachineStateSnapshot::Create(runtime);
    if (snapshot == 0u) return 0u;
    runtime.AssignStringRecord(snapshot + 4u,
                               runtime.ReadCString(runtime.ReadWord(object + 4u)));
    runtime.WriteWord(snapshot + 0x14u,
                      runtime.ReadWord(object + 0x184u));
    runtime.WriteWord(snapshot + 0x18u,
                      runtime.ReadWord(object + 0x198u));
    for (std::uint32_t index = 0; index < 3u; ++index) {
        runtime.WriteWord(snapshot + 0x1cu + 4u * index,
                          runtime.ReadWord(object + 0xacu + 4u * index));
        runtime.WriteWord(snapshot + 0x28u + 4u * index,
                          runtime.ReadWord(object + 0xa0u + 4u * index));
    }
    const auto touched = runtime.ReadWord(object + 0x2b4u);
    const auto action = runtime.ReadWord(object + 0x2b8u);
    runtime.WriteWord(snapshot + 0x38u,
                      touched != 0u && action != 0u ? 1u : 0u);
    runtime.WriteWord(snapshot + 0x34u, touched);
    runtime.WriteWord(snapshot + 0x3cu, action);
    runtime.AssignStringRecord(snapshot + 0x40u,
                               runtime.ReadStringRecord(object + 0x2bcu));
    return snapshot;
}

std::uint32_t GobMachine::ApplyStateSnapshotEvent(
    const std::uint32_t object, const std::uint32_t snapshot,
    IGobMachineSnapshotRuntime& runtime) {
    const auto enabled = runtime.ReadWord(object + 0x1a4u);
    if (enabled == 0u) return enabled;
    const auto type = runtime.SnapshotType(snapshot);
    if (type != 5u) return type;

    runtime.SetVisible(object, runtime.ReadWord(snapshot + 0x14u));
    runtime.WriteWord(object + 0x198u,
                      runtime.ReadWord(snapshot + 0x18u));
    runtime.SetPosition(object, snapshot + 0x1cu, 0u);
    runtime.SetRotation(object, snapshot + 0x28u, 0u);
    runtime.WriteWord(object + 0x2b4u,
                      runtime.ReadWord(snapshot + 0x34u));
    runtime.WriteWord(object + 0x2b8u,
                      runtime.ReadWord(snapshot + 0x3cu));
    runtime.AssignStringRecord(object + 0x2bcu,
                               runtime.ReadStringRecord(snapshot + 0x40u));
    const auto active_action = runtime.ReadWord(snapshot + 0x38u);
    if (active_action == 0u) return active_action;
    const auto state = object + kStateOffset;
    runtime.SelectAnimation(state,
                            runtime.ReadStringRecord(snapshot + 0x40u));
    runtime.SetAnimationTime(state, runtime.AnimationLength(state));
    return runtime.ClearAnimationFlag(state, 1u);
}

}  // namespace pal4
