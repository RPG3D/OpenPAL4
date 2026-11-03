#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

using GameDataRoleTableRecord = std::array<std::uint32_t, 80>;

class IGameDataRoleRowRuntime {
public:
    virtual ~IGameDataRoleRowRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CollectionSize(std::uint32_t address) = 0;
    virtual void LogBind(std::string_view prefix, std::uint32_t name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void InitializeStructures(std::uint32_t binding) = 0;
    virtual void BindEffects(std::uint32_t binding) = 0;
    virtual void InitializeRecord(GameDataRoleTableRecord& record) = 0;
    virtual void DestroyTemporaryRecord(GameDataRoleTableRecord& record) = 0;
    [[nodiscard]] virtual bool ParseBoundRow(
        std::uint32_t binding, std::uint32_t row,
        GameDataRoleTableRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t FindKey(
        std::uint32_t begin, std::uint32_t end, std::uint32_t key) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            const GameDataRoleTableRecord& record) = 0;
    virtual void InsertRecord(std::uint32_t array, std::uint32_t end,
                              const GameDataRoleTableRecord& record) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t position,
                           std::uint32_t key, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class IGameDataRoleAppendRuntime {
public:
    virtual ~IGameDataRoleAppendRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CollectionSize(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RowSize(std::uint32_t row) = 0;
    virtual void InitializeRecord(GameDataRoleTableRecord& record) = 0;
    virtual void DestroyTemporaryRecord(GameDataRoleTableRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t PropertyAt(
        std::uint32_t row, std::uint32_t index) = 0;
    virtual void ConvertField(std::uint32_t field,
                              GameDataRoleTableRecord& record,
                              std::uint32_t property) = 0;
    virtual void AppendRecord(std::uint32_t array, std::uint32_t end,
                              const GameDataRoleTableRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class IGameDataRoleRefreshRuntime {
public:
    virtual ~IGameDataRoleRefreshRuntime() = default;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Refresh(std::uint32_t table) = 0;
};

class GameDataRoleTable final {
public:
    // IDA 0x4ea390: bind, parse, validate and insert or replace a Role row.
    [[nodiscard]] static std::uint8_t AddOrReplace(
        std::uint32_t table, std::uint32_t row,
        IGameDataRoleRowRuntime& runtime);
    // IDA 0x4ea670: convert bound properties and append a Role record.
    static void Append(std::uint32_t table, std::uint32_t row,
                       IGameDataRoleAppendRuntime& runtime);
    // IDA 0x4ea820: mark dirty before rebuilding the index.
    [[nodiscard]] static std::uint32_t MarkAndRefresh(
        std::uint32_t table, IGameDataRoleRefreshRuntime& runtime);

private:
    [[nodiscard]] static bool NormalizeAndValidate(
        GameDataRoleTableRecord& record);
};

}  // namespace pal4
