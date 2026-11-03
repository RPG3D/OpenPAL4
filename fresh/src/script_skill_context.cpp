#include "pal4/script_skill_context.h"

#include <array>

namespace pal4 {

ScriptSkillContextState& ScriptSkillContext::Construct(
    ScriptSkillContextState& state, std::uint32_t skill,
    std::uint32_t context) {
    state.skill = skill;
    state.context = context;
    return state;
}

std::uint32_t ScriptSkillContext::Handle(
    const ScriptSkillContextState& state, std::uint32_t requested_id,
    std::uint32_t actor, std::uint32_t target,
    IScriptSkillContextRuntime& runtime) {
    if (state.skill == 0u || state.context == 0u ||
        requested_id != runtime.ReadWord(state.skill + 8u))
        return state.skill;

    const std::array actor_arguments{
        state.skill, requested_id, state.context, target};
    static_cast<void>(runtime.InvokeVirtual(actor, 280u, actor_arguments));
    const std::array target_arguments{
        state.context, state.skill, requested_id};
    static_cast<void>(runtime.InvokeVirtual(target, 308u, target_arguments));
    runtime.ProcessSkillChain(runtime.SkillSystemInstance(), state.skill,
                              actor, state.context, target);
    static_cast<void>(runtime.InvokeVirtual(actor, 284u, actor_arguments));
    return runtime.InvokeVirtual(target, 312u, target_arguments);
}

std::uint32_t ScriptSkillContext::FirstWord(
    const ScriptSkillContextState& state,
    IScriptSkillContextRuntime& runtime) {
    return state.skill == 0u ? 0xffffffffu
                             : runtime.ReadWord(state.skill);
}

std::uint32_t ScriptSkillContext::WordAt64(
    const ScriptSkillContextState& state,
    IScriptSkillContextRuntime& runtime) {
    return state.skill == 0u ? 0xffffffffu
                             : runtime.ReadWord(state.skill + 64u);
}

}  // namespace pal4
