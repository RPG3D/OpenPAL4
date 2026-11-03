#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataSkillBehaviorRuntime {
public:
    virtual ~IGameDataSkillBehaviorRuntime() = default;
    [[nodiscard]] virtual std::int32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual bool HasSkill(std::uint32_t catalog,
                                           std::int32_t skill) = 0;
    [[nodiscard]] virtual std::uint32_t FindSkill(std::uint32_t catalog,
                                      std::int32_t skill) = 0;
};

class GameDataSkillBehavior final {
public:
    // IDA 0x4d9be0: four signed member bounds.
    [[nodiscard]] static bool MembersInRange(
        std::uint32_t object, IGameDataSkillBehaviorRuntime& runtime);
    // IDA 0x4d9c20: resolve attached Skill after the original HasSkill gate.
    [[nodiscard]] static std::uint32_t ResolveAttachedSkill(
        std::uint32_t object, IGameDataSkillBehaviorRuntime& runtime);
    // IDA 0x4d9c50/0x4d9c60: member +0x88 access and predicate.
    [[nodiscard]] static std::int32_t MemberAt88(
        std::uint32_t object, IGameDataSkillBehaviorRuntime& runtime);
    [[nodiscard]] static bool MemberIsTwoOrFour(
        std::uint32_t object, IGameDataSkillBehaviorRuntime& runtime);
};

}  // namespace pal4
