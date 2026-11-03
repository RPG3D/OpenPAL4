#include "pal4/pal_actor_control_named_member.h"

namespace pal4 {

void IActorControlNamedMemberRuntime::ResetBaseVtable(std::uint32_t) {}
void IActorControlNamedMemberRuntime::Free(std::uint32_t) {}
void IActorControlNamedMemberRuntime::CallFirstMember(std::uint32_t) {}
void IActorControlNamedMemberRuntime::ResetMemberDisplay(std::uint32_t) {}

const char* PalActorControlNamedMember::FirstTag() {
    return "jl";
}

std::uint32_t PalActorControlNamedMember::Delete(
    const std::uint32_t object, const std::uint32_t flags,
    IActorControlNamedMemberRuntime& runtime) {
    runtime.ResetBaseVtable(object);
    if ((flags & 1u) != 0u) {
        runtime.Free(object);
    }
    return object;
}

const char* PalActorControlNamedMember::SecondTag() {
    return "jo";
}

std::uint32_t PalActorControlNamedMember::DispatchFirst(
    const PalActorControlNamedMemberPrefix& object, std::uint32_t,
    std::uint32_t, IActorControlNamedMemberRuntime& runtime) {
    if (object.member == 0u) return 0u;
    runtime.CallFirstMember(object.member);
    return 1u;
}

std::uint32_t PalActorControlNamedMember::DispatchSecond(
    const PalActorControlNamedMemberPrefix& object, std::uint32_t,
    std::uint32_t, IActorControlNamedMemberRuntime& runtime) {
    if (object.member == 0u) return 0u;
    runtime.ResetMemberDisplay(object.member);
    return 1u;
}

}  // namespace pal4
