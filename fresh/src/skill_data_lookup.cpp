#include "pal4/skill_data_lookup.h"

namespace pal4 {
namespace {

std::uint32_t FindRecord(std::uint32_t catalog, std::uint32_t table_offset,
                         std::uint32_t record_bytes, std::int32_t key,
                         ISkillDataLookupRuntime& runtime) {
    const auto table = runtime.ReadWord(catalog + table_offset);
    auto current = runtime.ReadWord(table + 8);
    const auto end = runtime.ReadWord(table + 12);
    auto count = (end - current) / 8;
    while (count != 0) {
        const auto half = count / 2;
        const auto middle = current + half * 8;
        if (static_cast<std::int32_t>(runtime.ReadWord(middle)) >= key) {
            count = half;
        } else {
            current = middle + 8;
            count -= half + 1;
        }
    }
    if (current == end ||
        key < static_cast<std::int32_t>(runtime.ReadWord(current)))
        return 0;
    const auto position = runtime.ReadWord(current + 4);
    return runtime.ReadWord(table + 96) + record_bytes * position;
}

}  // namespace

bool SkillDataLookup::IsSkillRecordValid(
    std::uint32_t record, ISkillDataLookupRuntime& runtime) {
    return runtime.ReadWord(record + 4) < 4 &&
           runtime.ReadWord(record + 8) <= 14;
}

bool SkillDataLookup::IsUnsetRecord(
    std::uint32_t record, ISkillDataLookupRuntime& runtime) {
    return runtime.ReadWord(record + 12) == 0xffffffffu;
}

std::uint32_t SkillDataLookup::FindSkill(
    std::uint32_t catalog, std::int32_t key,
    ISkillDataLookupRuntime& runtime) {
    return FindRecord(catalog, 4, 132, key, runtime);
}

bool SkillDataLookup::HasSkill(
    std::uint32_t catalog, std::int32_t key,
    ISkillDataLookupRuntime& runtime) {
    return FindSkill(catalog, key, runtime) != 0;
}

std::uint32_t SkillDataLookup::FindStunt(
    std::uint32_t catalog, std::int32_t key,
    ISkillDataLookupRuntime& runtime) {
    return FindRecord(catalog, 8, 208, key, runtime);
}

bool SkillDataLookup::HasStunt(
    std::uint32_t catalog, std::int32_t key,
    ISkillDataLookupRuntime& runtime) {
    return FindStunt(catalog, key, runtime) != 0;
}

std::uint32_t SkillDataLookup::FindMagic(
    std::uint32_t catalog, std::int32_t key,
    ISkillDataLookupRuntime& runtime) {
    return FindRecord(catalog, 12, 152, key, runtime);
}

bool SkillDataLookup::HasMagic(
    std::uint32_t catalog, std::int32_t key,
    ISkillDataLookupRuntime& runtime) {
    return FindMagic(catalog, key, runtime) != 0;
}

}  // namespace pal4
