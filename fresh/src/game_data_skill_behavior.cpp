#include "pal4/game_data_skill_behavior.h"

namespace pal4 {

bool GameDataSkillBehavior::MembersInRange(
    std::uint32_t object, IGameDataSkillBehaviorRuntime& runtime) {
    const auto member_88 = runtime.ReadWord(object + 0x88);
    if (member_88 < 0 || member_88 >= 6) return false;
    const auto member_04 = runtime.ReadWord(object + 4);
    if (member_04 < 0 || member_04 >= 7) return false;
    const auto member_84 = runtime.ReadWord(object + 0x84);
    if (member_84 < 0 || member_84 >= 3) return false;
    return runtime.ReadWord(object + 0x80) >= 0;
}

std::uint32_t GameDataSkillBehavior::ResolveAttachedSkill(
    std::uint32_t object, IGameDataSkillBehaviorRuntime& runtime) {
    const auto first_db = runtime.GameDatabase();
    const auto skill = runtime.ReadWord(object + 0x70);
    if (!runtime.HasSkill(first_db + 0x0c, skill)) return 0;
    const auto second_db = runtime.GameDatabase();
    return runtime.FindSkill(second_db + 0x0c, runtime.ReadWord(object + 0x70));
}

std::int32_t GameDataSkillBehavior::MemberAt88(
    std::uint32_t object, IGameDataSkillBehaviorRuntime& runtime) {
    return runtime.ReadWord(object + 0x88);
}

bool GameDataSkillBehavior::MemberIsTwoOrFour(
    std::uint32_t object, IGameDataSkillBehaviorRuntime& runtime) {
    const auto value = runtime.ReadWord(object + 0x88);
    return value == 2 || value == 4;
}

}  // namespace pal4
