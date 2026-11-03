#include "pal4/game_data_magic_behavior.h"

namespace pal4 {

bool GameDataMagicBehavior::MembersInRange(
    std::uint32_t object, IGameDataMagicBehaviorRuntime& runtime) {
    const auto member_08 = runtime.ReadWord(object + 8);
    if (member_08 < 0 || member_08 >= 7) return false;
    const auto member_04 = runtime.ReadWord(object + 4);
    if (member_04 < 0 || member_04 >= 5) return false;
    const auto member_50 = runtime.ReadWord(object + 0x50);
    return member_50 >= 0 && member_50 < 6;
}

std::uint32_t GameDataMagicBehavior::ResolveAttachedSkill(
    std::uint32_t object, IGameDataMagicBehaviorRuntime& runtime) {
    const auto first_db = runtime.GameDatabase();
    if (!runtime.HasSkill(first_db + 0x0c,
                             runtime.ReadWord(object + 0x48)))
        return 0;
    const auto second_db = runtime.GameDatabase();
    return runtime.FindSkill(second_db + 0x0c,
                           runtime.ReadWord(object + 0x48));
}

std::int32_t GameDataMagicBehavior::MemberAt50(
    std::uint32_t object, IGameDataMagicBehaviorRuntime& runtime) {
    return runtime.ReadWord(object + 0x50);
}

bool GameDataMagicBehavior::MemberIsTwoOrFour(
    std::uint32_t object, IGameDataMagicBehaviorRuntime& runtime) {
    const auto value = runtime.ReadWord(object + 0x50);
    return value == 2 || value == 4;
}

std::uint32_t GameDataMagicBehavior::InitializeTextFieldBase(
    std::uint32_t object, IGameDataMagicBehaviorRuntime& runtime) {
    runtime.WriteWord(object, 0x842bbc);
    return object;
}

}  // namespace pal4
