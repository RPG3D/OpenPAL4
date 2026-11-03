#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IGameDataMissionTableRuntime {
public:
    virtual ~IGameDataMissionTableRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t PreloadTable(
        std::uint32_t database, std::string_view name,
        std::uint32_t factory) = 0;
    [[nodiscard]] virtual std::uint32_t FindTable(
        std::uint32_t database, std::string_view name) = 0;
    virtual void LogError(std::string_view message) = 0;
};

using GameDataMissionRecord = std::array<std::uint32_t, 17>;

class IGameDataMissionRowRuntime {
public:
    virtual ~IGameDataMissionRowRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CollectionSize(std::uint32_t address) = 0;
    virtual void LogBind(std::string_view prefix,
                         std::uint32_t table_name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void InitializeStructures(std::uint32_t binding) = 0;
    virtual void BindEffects(std::uint32_t binding) = 0;
    virtual void InitializeRecord(GameDataMissionRecord& record) = 0;
    virtual void DestroyRecord(GameDataMissionRecord& record) = 0;
    [[nodiscard]] virtual bool ParseBoundRow(std::uint32_t binding,
                                             std::uint32_t row,
                                             GameDataMissionRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t FindKey(
        std::uint32_t begin, std::uint32_t end, std::uint32_t key) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            const GameDataMissionRecord& record) = 0;
    virtual void InsertRecord(std::uint32_t array, std::uint32_t end,
                              const GameDataMissionRecord& record) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t position,
                           std::uint32_t key, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::string DumpRow(std::uint32_t table,
                                              std::uint32_t row) = 0;
};

class IGameDataMissionAppendRuntime {
public:
    virtual ~IGameDataMissionAppendRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CollectionSize(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RowSize(std::uint32_t row) = 0;
    virtual void InitializeRecord(GameDataMissionRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyRecord(
        GameDataMissionRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t PropertyAt(std::uint32_t row,
                                                   std::uint32_t index) = 0;
    virtual void ConvertField(std::uint32_t field,
                              GameDataMissionRecord& record,
                              std::uint32_t property) = 0;
    virtual void AppendRecord(std::uint32_t array, std::uint32_t end,
                              const GameDataMissionRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(std::uint32_t table,
                                              std::uint32_t row) = 0;
};

class IGameDataMissionRefreshRuntime {
public:
    virtual ~IGameDataMissionRefreshRuntime() = default;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Refresh(std::uint32_t table) = 0;
};

class IGameDataMissionBindingRuntime {
public:
    virtual ~IGameDataMissionBindingRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void LogBind(std::string_view prefix,
                         std::uint32_t table_name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void InitializeStructures(std::uint32_t binding) = 0;
    virtual void BindEffects(std::uint32_t binding) = 0;
};

class GameDataMissionTable final {
public:
    // IDA 0x4db120: signed-key binary lookup in 8-byte index entries.
    [[nodiscard]] static std::uint32_t FindRecord(
        std::uint32_t manager, std::int32_t key,
        IGameDataMissionTableRuntime& runtime);
    // IDA 0x4db190: preload Mission through the table factory.
    [[nodiscard]] static std::uint8_t Preload(
        std::uint32_t manager, IGameDataMissionTableRuntime& runtime);
    // IDA 0x4db1b0: bind the Mission table or log and throw.
    [[nodiscard]] static std::uint8_t Verify(
        std::uint32_t manager, IGameDataMissionTableRuntime& runtime);
    // IDA 0x4db260: signed descending comparison of member +8.
    [[nodiscard]] static bool CompareMemberAt8(
        std::uint32_t left, std::uint32_t right,
        IGameDataMissionTableRuntime& runtime);
    // IDA 0x4dba20: bind, parse and insert or replace a Mission row.
    [[nodiscard]] static std::uint8_t AddOrReplace(
        std::uint32_t table, std::uint32_t row,
        IGameDataMissionRowRuntime& runtime);
    // IDA 0x4dbcd0: convert bound fields and append one Mission record.
    [[nodiscard]] static std::uint32_t Append(
        std::uint32_t table, std::uint32_t row,
        IGameDataMissionAppendRuntime& runtime);
    // IDA 0x4dbea0: mark dirty before refreshing the Mission table.
    [[nodiscard]] static std::uint32_t MarkAndRefresh(
        std::uint32_t table, IGameDataMissionRefreshRuntime& runtime);
    // IDA 0x4dc820: refresh Effect binding when pointer/field counts differ.
    static void RefreshBinding(std::uint32_t table,
                               IGameDataMissionBindingRuntime& runtime);
};

}  // namespace pal4
