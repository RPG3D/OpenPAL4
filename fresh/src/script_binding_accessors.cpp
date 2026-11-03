#include "pal4/script_binding_accessors.h"

namespace pal4 {

std::int32_t ScriptBindingAccessors::GetInteger(
    const ScriptEnumObjectState& state, std::uint32_t argument,
    IScriptBindingAccessorRuntime& runtime) {
    return state.words[2] == 0u ? 0 :
           runtime.InvokeInteger(state.words[2], argument);
}

std::int32_t ScriptBindingAccessors::SetInteger(
    const ScriptEnumObjectState& state, std::uint32_t first,
    std::uint32_t second, std::int32_t incoming_eax,
    IScriptBindingAccessorRuntime& runtime) {
    return state.words[4] == 0u ? incoming_eax :
           runtime.InvokeSetter(state.words[4], first, second);
}

double ScriptBindingAccessors::GetFloating(
    const ScriptEnumObjectState& state, std::uint32_t argument,
    IScriptBindingAccessorRuntime& runtime) {
    return state.words[2] == 0u ? 0.0 :
           runtime.InvokeFloating(state.words[2], argument);
}

std::int32_t ScriptBindingAccessors::SetFloating(
    const ScriptEnumObjectState& state, std::uint32_t first,
    std::uint32_t second, std::int32_t incoming_eax,
    IScriptBindingAccessorRuntime& runtime) {
    return state.words[4] == 0u ? incoming_eax :
           runtime.InvokeSetter(state.words[4], first, second);
}

std::int32_t ScriptBindingAccessors::GetRoleField(
    const ScriptRoleBindingState& state, std::uint32_t argument,
    IScriptBindingAccessorRuntime& runtime) {
    return state.words[4] == 0u ? 0 :
           runtime.InvokeInteger(state.words[4], argument);
}

std::int32_t ScriptBindingAccessors::SetRoleField(
    const ScriptRoleBindingState& state, std::uint32_t first,
    std::uint32_t second, std::int32_t incoming_eax,
    IScriptBindingAccessorRuntime& runtime) {
    return state.words[6] == 0u ? incoming_eax :
           runtime.InvokeSetter(state.words[6], first, second);
}

}  // namespace pal4
