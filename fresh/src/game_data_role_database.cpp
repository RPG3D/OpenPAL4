#include "pal4/game_data_role_database.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace pal4 {

std::uint32_t GameDataRoleDatabase::FindRoleRecord(
    std::span<const GameDataRoleIndexEntry> index,
    std::uint32_t records_begin, std::int32_t id) {
    const auto key = static_cast<std::int32_t>(
        static_cast<std::uint32_t>(id) + 1001u);
    std::size_t first = 0;
    std::size_t count = index.size();
    while (count != 0) {
        const auto half = count / 2;
        if (index[first + half].key >= key) {
            count = half;
        } else {
            first += half + 1;
            count -= half + 1;
        }
    }
    if (first == index.size() || index[first].key != key) return 0;
    return records_begin + 320u * index[first].record_index;
}

std::uint8_t GameDataRoleDatabase::RegisterSchemas(
    IGameDataRoleDatabaseRuntime& runtime) const {
    runtime.RegisterSchema("Role", 0x4e9fc0);
    runtime.RegisterSchema("UpgradeData", 0x4eac30);
    return runtime.RegisterSchema("CombatPrize", 0x4eb670);
}

std::uint8_t GameDataRoleDatabase::Load(
    IGameDataRoleDatabaseRuntime& runtime) {
    const auto require = [&runtime](std::uint32_t value,
                                    std::string_view name) {
        if (value == 0) {
            runtime.ReportMissing(name);
            throw std::runtime_error("missing PAL DB Role table: " +
                                     std::string(name));
        }
    };
    role_ = runtime.FindTable("Role");
    require(role_, "Role");
    combat_prize_ = runtime.FindTable("CombatPrize");
    require(combat_prize_, "CombatPrize");
    upgrade_data_ = runtime.FindTable("UpgradeData");
    require(upgrade_data_, "UpgradeData");
    race_ = runtime.FindProperty("Race");
    require(race_, "Race");
    return 1;
}

bool GameDataUpgradeSelector::MatchesRole(
    const GameDataUpgradeRecordView& record, std::uint32_t role_id) {
    return record.role_id == role_id;
}

std::uint32_t GameDataUpgradeSelector::FindNextRecord(
    std::span<const GameDataUpgradeRecordView> records,
    std::uint32_t role_id, std::int32_t threshold) {
    std::int32_t best_level = 100;
    std::uint32_t best_address = 0;
    for (const auto& record : records) {
        if (MatchesRole(record, role_id) && record.value > threshold &&
            record.level < best_level) {
            best_level = record.level;
            best_address = record.address;
        }
    }
    return best_address;
}

std::int32_t GameDataUpgradeSelector::PreviousValue(
    std::span<const GameDataUpgradeRecordView> records,
    std::uint32_t role_id, std::int32_t level) {
    for (const auto& record : records)
        if (MatchesRole(record, role_id) && record.level == level)
            return static_cast<std::int32_t>(
                static_cast<std::uint32_t>(record.value) - 1u);
    return 0;
}

std::int32_t GameDataUpgradeSelector::NextValue(
    std::span<const GameDataUpgradeRecordView> records,
    std::uint32_t role_id, std::int32_t level) {
    if (level <= 0) return 0;
    for (const auto& record : records)
        if (MatchesRole(record, role_id) && record.level == level - 1)
            return record.value;
    return 0;
}

}  // namespace pal4
