#pragma once

#include <cstdint>

namespace pal4 {

class ISkillDataLookupRuntime {
public:
    virtual ~ISkillDataLookupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
};

class SkillDataLookup final {
public:
    // IDA 0x4f3810 and 0x4f3830: field predicates on a skill record.
    [[nodiscard]] static bool IsSkillRecordValid(
        std::uint32_t record, ISkillDataLookupRuntime& runtime);
    [[nodiscard]] static bool IsUnsetRecord(
        std::uint32_t record, ISkillDataLookupRuntime& runtime);

    // IDA 0x4f3870/0x4f38e0: 132-byte Skill record lookup.
    [[nodiscard]] static std::uint32_t FindSkill(
        std::uint32_t catalog, std::int32_t key,
        ISkillDataLookupRuntime& runtime);
    [[nodiscard]] static bool HasSkill(
        std::uint32_t catalog, std::int32_t key,
        ISkillDataLookupRuntime& runtime);
    // IDA 0x4f3960/0x4f39d0: 208-byte Stunt record lookup.
    [[nodiscard]] static std::uint32_t FindStunt(
        std::uint32_t catalog, std::int32_t key,
        ISkillDataLookupRuntime& runtime);
    [[nodiscard]] static bool HasStunt(
        std::uint32_t catalog, std::int32_t key,
        ISkillDataLookupRuntime& runtime);
    // IDA 0x4f3a50/0x4f3ac0: 152-byte Magic record lookup.
    [[nodiscard]] static std::uint32_t FindMagic(
        std::uint32_t catalog, std::int32_t key,
        ISkillDataLookupRuntime& runtime);
    [[nodiscard]] static bool HasMagic(
        std::uint32_t catalog, std::int32_t key,
        ISkillDataLookupRuntime& runtime);
};

}  // namespace pal4
