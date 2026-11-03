#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

using GameDataUpgradeTableRecord = std::array<std::uint32_t, 40>;

class IGameDataUpgradeRowRuntime {
public:
    virtual ~IGameDataUpgradeRowRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CollectionSize(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RowSize(std::uint32_t row) = 0;
    virtual void LogBind(std::string_view prefix, std::uint32_t name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void InitializeStructures(std::uint32_t binding) = 0;
    virtual void BindEffects(std::uint32_t binding) = 0;
    virtual void InitializeRecord(GameDataUpgradeTableRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t PropertyAt(
        std::uint32_t row, std::uint32_t index) = 0;
    virtual void ConvertField(std::uint32_t field,
                              GameDataUpgradeTableRecord& record,
                              std::uint32_t property) = 0;
    [[nodiscard]] virtual std::uint32_t FindKey(
        std::uint32_t begin, std::uint32_t end, std::uint32_t key) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            const GameDataUpgradeTableRecord& record) = 0;
    virtual void InsertRecord(std::uint32_t array, std::uint32_t end,
                              const GameDataUpgradeTableRecord& record) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t position,
                           std::uint32_t key, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class IGameDataUpgradeAppendRuntime {
public:
    virtual ~IGameDataUpgradeAppendRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CollectionSize(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RowSize(std::uint32_t row) = 0;
    virtual void InitializeRecord(GameDataUpgradeTableRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t PropertyAt(
        std::uint32_t row, std::uint32_t index) = 0;
    virtual void ConvertField(std::uint32_t field,
                              GameDataUpgradeTableRecord& record,
                              std::uint32_t property) = 0;
    [[nodiscard]] virtual std::uint32_t AppendRecord(
        std::uint32_t array, std::uint32_t end,
        const GameDataUpgradeTableRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class IGameDataUpgradeRefreshRuntime {
public:
    virtual ~IGameDataUpgradeRefreshRuntime() = default;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Refresh(std::uint32_t table) = 0;
};

class GameDataUpgradeTable final {
public:
    // IDA 0x4eae40: bind, convert, validate, then insert or replace a row.
    [[nodiscard]] static std::uint8_t AddOrReplace(
        std::uint32_t table, std::uint32_t row,
        IGameDataUpgradeRowRuntime& runtime);
    // IDA 0x4eb150: convert and append one 160-byte record.
    [[nodiscard]] static std::uint32_t Append(
        std::uint32_t table, std::uint32_t row,
        IGameDataUpgradeAppendRuntime& runtime);
    // IDA 0x4eb2b0: mark dirty and refresh the table.
    [[nodiscard]] static std::uint32_t MarkAndRefresh(
        std::uint32_t table, IGameDataUpgradeRefreshRuntime& runtime);

private:
    [[nodiscard]] static bool HasValidRoleId(
        const GameDataUpgradeTableRecord& record);
};

}  // namespace pal4
