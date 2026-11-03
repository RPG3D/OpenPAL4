#include "pal4/script_role_binding.h"

namespace pal4 {
namespace {

std::uint32_t Construct(ScriptRoleBindingState& state,
                        std::uint32_t object_address,
                        std::uint32_t source, std::uint32_t count,
                        std::uint32_t vtable) {
    state.words[1] = source;
    state.words[2] = count;
    state.words[3] &= 0xffffff00u;
    state.words[4] = 0u;
    state.words[5] &= 0xffffff00u;
    state.words[6] = 0u;
    state.words[7] = (state.words[7] & 0xffffff00u) | (source & 0xffu);
    state.words[8] = 0u;
    state.words[9] = 0u;
    state.words[10] = 0u;
    state.words[0] = vtable;
    return object_address;
}

void ClearAdapter(ScriptRoleBindingState& state, std::uint32_t flag_word,
                  IScriptEnumObjectLifecycleRuntime& runtime) {
    const auto pointer = state.words[flag_word + 1u];
    if (pointer != 0u) {
        if (static_cast<std::uint8_t>(state.words[flag_word]) != 0u)
            runtime.DestroyAdapter(pointer, 1u);
        state.words[flag_word] &= 0xffffff00u;
    }
    state.words[flag_word + 1u] = 0u;
}

std::uint8_t Destroy(ScriptRoleBindingState& state,
                     std::uint32_t derived_vtable,
                     IScriptEnumObjectLifecycleRuntime& runtime) {
    state.words[0] = derived_vtable;
    ClearAdapter(state, 3u, runtime);
    ClearAdapter(state, 5u, runtime);

    const auto data = state.words[8];
    if (data != 0u) {
        const auto reference = runtime.ReadByte(data - 1u);
        if (reference == 0u || reference == 0xffu)
            runtime.Free(data - 1u);
        else
            runtime.WriteByte(data - 1u,
                              static_cast<std::uint8_t>(reference - 1u));
    }
    state.words[8] = 0u;
    state.words[9] = 0u;
    state.words[10] = 0u;
    const auto result = static_cast<std::uint8_t>(state.words[3]);
    state.words[0] = ScriptEnumObjectLifecycle::kBaseVtable;
    return result;
}

}  // namespace

std::uint32_t ScriptRoleBinding::ConstructFieldThree(
    ScriptRoleBindingState& state, std::uint32_t object_address,
    std::uint32_t source, std::uint32_t count) {
    return Construct(state, object_address, source, count, 0x848218u);
}

std::uint32_t ScriptRoleBinding::ConstructFieldFour(
    ScriptRoleBindingState& state, std::uint32_t object_address,
    std::uint32_t source, std::uint32_t count) {
    return Construct(state, object_address, source, count, 0x848230u);
}

std::uint32_t ScriptRoleBinding::ConstructFieldFive(
    ScriptRoleBindingState& state, std::uint32_t object_address,
    std::uint32_t source, std::uint32_t count) {
    return Construct(state, object_address, source, count, 0x848248u);
}

std::uint32_t ScriptRoleBinding::ConstructFieldSix(
    ScriptRoleBindingState& state, std::uint32_t object_address,
    std::uint32_t source, std::uint32_t count) {
    return Construct(state, object_address, source, count, 0x848260u);
}

std::uint32_t ScriptRoleBinding::ConstructFieldSeven(
    ScriptRoleBindingState& state, std::uint32_t object_address,
    std::uint32_t source, std::uint32_t count) {
    return Construct(state, object_address, source, count, 0x848278u);
}

std::uint32_t ScriptRoleBinding::ConstructFieldEight(
    ScriptRoleBindingState& state, std::uint32_t object_address,
    std::uint32_t source, std::uint32_t count) {
    return Construct(state, object_address, source, count, 0x848290u);
}

std::uint32_t ScriptRoleBinding::ConstructRace(
    ScriptRoleBindingState& state, std::uint32_t object_address,
    std::uint32_t source, std::uint32_t count) {
    return Construct(state, object_address, source, count, 0x8482a8u);
}

std::uint32_t ScriptRoleBinding::NameAddress(
    std::uint32_t object_address) {
    return object_address + 28u;
}

std::uint8_t ScriptRoleBinding::DestroyCombatPosition(
    ScriptRoleBindingState& state,
    IScriptEnumObjectLifecycleRuntime& runtime) {
    return Destroy(state, 0x8481e8u, runtime);
}

std::uint8_t ScriptRoleBinding::DestroyStandaloneOne(
    ScriptRoleBindingState& state,
    IScriptEnumObjectLifecycleRuntime& runtime) {
    return Destroy(state, 0x8481b8u, runtime);
}

std::uint8_t ScriptRoleBinding::DestroyStandaloneTwo(
    ScriptRoleBindingState& state,
    IScriptEnumObjectLifecycleRuntime& runtime) {
    return Destroy(state, 0x8481a0u, runtime);
}

std::uint8_t ScriptRoleBinding::DestroyStandaloneThree(
    ScriptRoleBindingState& state,
    IScriptEnumObjectLifecycleRuntime& runtime) {
    return Destroy(state, 0x848188u, runtime);
}

std::uint8_t ScriptRoleBinding::DestroyFieldThree(
    ScriptRoleBindingState& state,
    IScriptEnumObjectLifecycleRuntime& runtime) {
    return Destroy(state, 0x848218u, runtime);
}

std::uint8_t ScriptRoleBinding::DestroyFieldFour(
    ScriptRoleBindingState& state,
    IScriptEnumObjectLifecycleRuntime& runtime) {
    return Destroy(state, 0x848230u, runtime);
}

std::uint8_t ScriptRoleBinding::DestroyFieldFive(
    ScriptRoleBindingState& state,
    IScriptEnumObjectLifecycleRuntime& runtime) {
    return Destroy(state, 0x848248u, runtime);
}

std::uint8_t ScriptRoleBinding::DestroyFieldSix(
    ScriptRoleBindingState& state,
    IScriptEnumObjectLifecycleRuntime& runtime) {
    return Destroy(state, 0x848260u, runtime);
}

std::uint8_t ScriptRoleBinding::DestroyFieldSeven(
    ScriptRoleBindingState& state,
    IScriptEnumObjectLifecycleRuntime& runtime) {
    return Destroy(state, 0x848278u, runtime);
}

std::uint8_t ScriptRoleBinding::DestroyFieldEight(
    ScriptRoleBindingState& state,
    IScriptEnumObjectLifecycleRuntime& runtime) {
    return Destroy(state, 0x848290u, runtime);
}

std::uint8_t ScriptRoleBinding::DestroyRace(
    ScriptRoleBindingState& state,
    IScriptEnumObjectLifecycleRuntime& runtime) {
    return Destroy(state, 0x8482a8u, runtime);
}

}  // namespace pal4
