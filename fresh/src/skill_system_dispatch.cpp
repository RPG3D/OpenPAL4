#include "pal4/skill_system_dispatch.h"

namespace pal4 {

std::uint32_t SkillSystemDispatch::FindCallback(
    std::uint32_t system, std::int32_t skill_id,
    ISkillSystemDispatchRuntime& runtime) {
    auto cursor = runtime.ReadWord(system + kVectorBeginOffset);
    const auto end = runtime.ReadWord(system + kVectorEndOffset);
    auto remaining = static_cast<std::int32_t>(end - cursor) >> 3;
    while (remaining > 0) {
        const auto half = remaining / 2;
        const auto midpoint = cursor + static_cast<std::uint32_t>(half) * 8u;
        if (static_cast<std::int32_t>(runtime.ReadWord(midpoint)) >= skill_id) {
            remaining = half;
        } else {
            cursor = midpoint + 8u;
            remaining -= half + 1;
        }
    }
    if (cursor == end ||
        skill_id < static_cast<std::int32_t>(runtime.ReadWord(cursor)))
        return 0u;
    return runtime.ReadWord(cursor + 4u);
}

std::uint32_t SkillSystemDispatch::ProcessChain(
    std::uint32_t system, std::uint32_t skill, std::uint32_t context,
    std::uint32_t actor, std::uint32_t target,
    ISkillSystemDispatchRuntime& runtime) {
    auto skill_id = runtime.ReadWord(skill + kSkillIdOffset);
    auto callback = FindCallback(system, static_cast<std::int32_t>(skill_id),
                                 runtime);
    while (callback != 0u) {
        skill_id = runtime.ReadWord(skill + kSkillIdOffset);
        runtime.ActorEvent(288u, actor, skill, skill_id, target);
        skill_id = runtime.ReadWord(skill + kSkillIdOffset);
        runtime.TargetEvent(316u, target, actor, skill, skill_id);
        runtime.InvokeCallback(callback, skill, context, actor, target);
        skill_id = runtime.ReadWord(skill + kSkillIdOffset);
        runtime.ActorEvent(292u, actor, skill, skill_id, target);
        skill_id = runtime.ReadWord(skill + kSkillIdOffset);
        runtime.TargetEvent(320u, target, actor, skill, skill_id);

        const auto next_id = runtime.ReadWord(skill + kNextSkillIdOffset);
        if (next_id == 0u) break;
        skill = runtime.LookupSkill(next_id);
        if (skill == 0u) break;
        skill_id = runtime.ReadWord(skill + kSkillIdOffset);
        callback = FindCallback(system, static_cast<std::int32_t>(skill_id),
                                runtime);
    }
    return 0u;
}

}  // namespace pal4
