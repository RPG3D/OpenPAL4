#include "pal4/actor_manager_string_method_property.h"

namespace pal4 {
namespace {

void AssignEmptyOwnedSlot(const std::uint32_t slot,
                          IActorManagerStringMethodRuntime& runtime) {
    const auto object = runtime.ReadWord(slot + 4u);
    if (object) {
        if (runtime.ReadByte(slot)) runtime.DeleteObject(object, 1u);
        runtime.WriteByte(slot, 0u);
    }
    runtime.WriteWord(slot + 4u, 0u);
}

void DestroyOwnedSlot(const std::uint32_t slot,
                      IActorManagerStringMethodRuntime& runtime) {
    if (runtime.ReadByte(slot)) {
        const auto object = runtime.ReadWord(slot + 4u);
        if (object) runtime.DeleteObject(object, 1u);
    }
}

void ReleaseText(const std::uint32_t record,
                 IActorManagerStringMethodRuntime& runtime) {
    const auto text = runtime.ReadWord(record + 4u);
    if (text) {
        const auto header = text - 1u;
        const auto references = runtime.ReadByte(header);
        if (references == 0u || references == 0xffu)
            runtime.Free(header);
        else
            runtime.WriteByte(header,
                              static_cast<std::uint8_t>(references - 1u));
    }
    runtime.WriteWord(record + 4u, 0u);
    runtime.WriteWord(record + 8u, 0u);
    runtime.WriteWord(record + 12u, 0u);
}

}  // namespace

void ActorManagerStringMethodProperty::Destroy(
    const std::uint32_t object,
    IActorManagerStringMethodRuntime& runtime) {
    DestroyWithVtable(object, kDerivedVtable, runtime);
}

void ActorManagerStringMethodProperty::DestroyWithVtable(
    const std::uint32_t object, const std::uint32_t derived_vtable,
    IActorManagerStringMethodRuntime& runtime) {
    runtime.WriteWord(object, derived_vtable);
    AssignEmptyOwnedSlot(object + 4u, runtime);
    AssignEmptyOwnedSlot(object + 12u, runtime);
    ReleaseText(object + 20u, runtime);
    AssignEmptyOwnedSlot(object + 4u, runtime);
    AssignEmptyOwnedSlot(object + 12u, runtime);
    DestroyOwnedSlot(object + 12u, runtime);
    DestroyOwnedSlot(object + 4u, runtime);
    runtime.WriteWord(object, kBaseVtable);
}

void ActorManagerStringMethodProperty::CleanupOwnedPair(
    const std::uint32_t pair,
    IActorManagerStringMethodRuntime& runtime) {
    AssignEmptyOwnedSlot(pair, runtime);
    AssignEmptyOwnedSlot(pair + 8u, runtime);
    DestroyOwnedSlot(pair + 8u, runtime);
    DestroyOwnedSlot(pair, runtime);
}

void ActorManagerStringMethodProperty::ReleaseOwnedSlot(
    const std::uint32_t slot,
    IActorManagerStringMethodRuntime& runtime) {
    DestroyOwnedSlot(slot, runtime);
}

}  // namespace pal4
