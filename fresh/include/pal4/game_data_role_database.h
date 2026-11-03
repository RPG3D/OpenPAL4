#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace pal4 {

struct GameDataRoleIndexEntry {
    std::int32_t key = 0;
    std::uint32_t record_index = 0;
};

class IGameDataRoleDatabaseRuntime {
public:
    virtual ~IGameDataRoleDatabaseRuntime() = default;
    virtual std::uint8_t RegisterSchema(std::string_view name,
                                        std::uint32_t constructor) = 0;
    virtual std::uint32_t FindTable(std::string_view name) = 0;
    virtual std::uint32_t FindProperty(std::string_view name) = 0;
    virtual void ReportMissing(std::string_view name) = 0;
};

class GameDataRoleDatabase final {
public:
    // IDA 0x4e9930: lower-bound lookup of id + 1001 in the Role index.
    [[nodiscard]] static std::uint32_t FindRoleRecord(
        std::span<const GameDataRoleIndexEntry> index,
        std::uint32_t records_begin, std::int32_t id);
    // IDA 0x4e99a0: register Role, UpgradeData and CombatPrize schemas.
    [[nodiscard]] std::uint8_t RegisterSchemas(
        IGameDataRoleDatabaseRuntime& runtime) const;
    // IDA 0x4e99e0: resolve Role, CombatPrize, UpgradeData and Race.
    [[nodiscard]] std::uint8_t Load(IGameDataRoleDatabaseRuntime& runtime);

    [[nodiscard]] std::uint32_t role() const { return role_; }
    [[nodiscard]] std::uint32_t race() const { return race_; }
    [[nodiscard]] std::uint32_t upgrade_data() const { return upgrade_data_; }
    [[nodiscard]] std::uint32_t combat_prize() const { return combat_prize_; }

private:
    std::uint32_t role_ = 0;
    std::uint32_t race_ = 0;
    std::uint32_t upgrade_data_ = 0;
    std::uint32_t combat_prize_ = 0;
};

struct GameDataUpgradeRecordView {
    std::uint32_t address = 0;
    std::uint32_t role_id = 0;  // native record +148
    std::int32_t level = 0;     // native record +152
    std::int32_t value = 0;     // native record +156
};

class GameDataUpgradeSelector final {
public:
    // IDA 0x4e9fa0: matches the record's role ID at +148.
    [[nodiscard]] static bool MatchesRole(const GameDataUpgradeRecordView& record,
                                          std::uint32_t role_id);
    // IDA 0x4e9bf0: lowest level below 100 whose value exceeds threshold.
    [[nodiscard]] static std::uint32_t FindNextRecord(
        std::span<const GameDataUpgradeRecordView> records,
        std::uint32_t role_id, std::int32_t threshold);
    // IDA 0x4e9d20: value minus one for the first matching level.
    [[nodiscard]] static std::int32_t PreviousValue(
        std::span<const GameDataUpgradeRecordView> records,
        std::uint32_t role_id, std::int32_t level);
    // IDA 0x4e9e60: value for the first matching previous level.
    [[nodiscard]] static std::int32_t NextValue(
        std::span<const GameDataUpgradeRecordView> records,
        std::uint32_t role_id, std::int32_t level);
};

}  // namespace pal4
