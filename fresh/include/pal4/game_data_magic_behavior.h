#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataMagicBehaviorRuntime {
public:
    virtual ~IGameDataMagicBehaviorRuntime() = default;
    [[nodiscard]] virtual std::int32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual bool HasSkill(std::uint32_t catalog,
                                           std::int32_t magic) = 0;
    [[nodiscard]] virtual std::uint32_t FindSkill(std::uint32_t catalog,
                                      std::int32_t magic) = 0;
};

class GameDataMagicBehavior final {
public:
    // IDA 0x4da990: three signed member bounds.
    [[nodiscard]] static bool MembersInRange(
        std::uint32_t object, IGameDataMagicBehaviorRuntime& runtime);
    // IDA 0x4da9c0: resolve the attached Skill after the original HasSkill gate.
    [[nodiscard]] static std::uint32_t ResolveAttachedSkill(
        std::uint32_t object, IGameDataMagicBehaviorRuntime& runtime);
    // IDA 0x4da9f0/0x4daa00: member +0x50 access and predicate.
    [[nodiscard]] static std::int32_t MemberAt50(
        std::uint32_t object, IGameDataMagicBehaviorRuntime& runtime);
    [[nodiscard]] static bool MemberIsTwoOrFour(
        std::uint32_t object, IGameDataMagicBehaviorRuntime& runtime);
    // IDA 0x4daa20: base text-field constructor writes its vtable.
    [[nodiscard]] static std::uint32_t InitializeTextFieldBase(
        std::uint32_t object, IGameDataMagicBehaviorRuntime& runtime);
};

}  // namespace pal4
