#include "pal4/object_movement_reset.h"

namespace pal4 {

void ObjectMovementResetRuntimeStub::CallSlot24(ObjectMovementState&) {}
void ObjectMovementResetRuntimeStub::CallScalarSetter(
    ObjectMovementState&, std::uint32_t, std::uint32_t) {}
std::array<std::uint32_t, 3> ObjectMovementResetRuntimeStub::GlobalVector() {
    return {0u, 0u, 0u};
}
std::uint32_t ObjectMovementResetRuntimeStub::MemberVtable(
    std::uint32_t) { return 0u; }
std::uint32_t ObjectMovementResetRuntimeStub::CallSlot18(
    ObjectMovementState&, std::array<std::uint32_t, 3>&) { return 0u; }
std::uint32_t ObjectMovementResetRuntimeStub::CallMemberSlot4(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }

std::uint32_t ObjectMovementReset::Reset(
    ObjectMovementState& state, IObjectMovementResetRuntime& runtime) {
    runtime.CallSlot24(state);
    state.words[13] = 0u;
    state.words[14] = 0u;
    state.words[21] &= 0xffffff00u;

    runtime.CallScalarSetter(state, 0x54u, 0x3f800000u);
    runtime.CallScalarSetter(state, 0x68u, 0u);
    runtime.CallScalarSetter(state, 0x5cu, 0x3f000000u);
    runtime.CallScalarSetter(state, 0x74u, 0x3dcccccdu);
    runtime.CallScalarSetter(state, 0x7cu, 0x3f800000u);

    const auto vector = runtime.GlobalVector();
    for (std::uint32_t index = 0; index < 3u; ++index) {
        state.words[35u + index] = vector[index];
        state.words[29u + index] = vector[index];
        state.words[32u + index] = vector[index];
    }
    state.words[28] = 0u;
    state.words[38] = 0u;
    for (std::uint32_t index = 0; index < 3u; ++index)
        state.words[39u + index] = vector[index];

    const auto member_vtable = runtime.MemberVtable(state.words[42]);
    std::array<std::uint32_t, 3> output{};
    const auto token = runtime.CallSlot18(state, output);
    return runtime.CallMemberSlot4(member_vtable, state.words[42], token);
}

}  // namespace pal4
