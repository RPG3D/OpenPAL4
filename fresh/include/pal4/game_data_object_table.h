#pragma once

#include "pal4/game_data_field_schema.h"
#include "pal4/legacy_crc32.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

using GameDataObjectRecord = std::array<std::uint32_t, 14>;

class IGameDataObjectRowRuntime : public IGameDataFieldSchemaRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CollectionSize(
        std::uint32_t address) = 0;
    virtual void LogBind(std::string_view prefix,
                         std::uint32_t name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void BindEffects(std::uint32_t binding) = 0;
    virtual void InitializeRecord(GameDataObjectRecord& record) = 0;
    virtual void DestroyTemporaryRecord(GameDataObjectRecord& record) = 0;
    [[nodiscard]] virtual bool ParseBoundRow(
        std::uint32_t binding, std::uint32_t row,
        GameDataObjectRecord& record) = 0;
    [[nodiscard]] virtual const std::uint8_t* RecordTextData(
        const GameDataObjectRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t FindKey(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t key) = 0;
    virtual void InsertRecord(std::uint32_t array, std::uint32_t end,
                              const GameDataObjectRecord& record) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t position,
                           std::uint32_t key, std::uint32_t index) = 0;
    virtual void AssignRecord(std::uint32_t destination,
                              const GameDataObjectRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class IGameDataObjectAppendRuntime {
public:
    virtual ~IGameDataObjectAppendRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CollectionSize(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RowSize(std::uint32_t row) = 0;
    virtual void InitializeRecord(GameDataObjectRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupTemporaryRecord(
        GameDataObjectRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t PropertyAt(
        std::uint32_t row, std::uint32_t index) = 0;
    virtual void ConvertField(std::uint32_t field,
                              GameDataObjectRecord& record,
                              std::uint32_t property) = 0;
    [[nodiscard]] virtual const std::uint8_t* RecordTextData(
        const GameDataObjectRecord& record) = 0;
    virtual void AppendRecord(std::uint32_t array, std::uint32_t end,
                              const GameDataObjectRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class IGameDataObjectRefreshRuntime {
public:
    virtual ~IGameDataObjectRefreshRuntime() = default;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Refresh(
        std::uint32_t table) = 0;
};

class IGameDataObjectBindingRefreshRuntime
    : public IGameDataFieldSchemaRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void LogBind(std::string_view prefix,
                         std::uint32_t name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void BindEffects(std::uint32_t binding) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseLogBuffer() = 0;
};

class GameDataObjectTable final {
public:
    // IDA 0x4ef480: bind fields, parse, validate and insert/replace one row.
    [[nodiscard]] static std::uint8_t AddOrReplace(
        std::uint32_t table, std::uint32_t row,
        IGameDataObjectRowRuntime& runtime, LegacyCrc32& crc);
    // IDA 0x4ef760: convert bound fields and append a 56-byte record.
    [[nodiscard]] static std::uint32_t Append(
        std::uint32_t table, std::uint32_t row,
        IGameDataObjectAppendRuntime& runtime, LegacyCrc32& crc);
    // IDA 0x4ef920: mark the table dirty and run its refresh method.
    [[nodiscard]] static std::uint32_t MarkAndRefresh(
        std::uint32_t table, IGameDataObjectRefreshRuntime& runtime);
    // IDA 0x4f13e0: refresh field bindings when source counts diverge.
    [[nodiscard]] static std::uint32_t RefreshBindings(
        std::uint32_t table, IGameDataObjectBindingRefreshRuntime& runtime);
};

}  // namespace pal4
