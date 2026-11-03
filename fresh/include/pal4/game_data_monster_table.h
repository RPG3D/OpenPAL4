#pragma once

#include <cstdint>
#include <array>
#include <string>
#include <string_view>

namespace pal4 {

class IGameDataMonsterTableRuntime {
public:
    virtual ~IGameDataMonsterTableRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t PreloadTable(
        std::uint32_t database, std::string_view name,
        std::uint32_t factory) = 0;
    [[nodiscard]] virtual std::uint32_t FindTable(
        std::uint32_t database, std::string_view name) = 0;
    virtual void LogError(std::string_view message) = 0;
};

using GameDataMonsterRecord = std::array<std::uint32_t, 149>;

class IGameDataMonsterRowRuntime {
public:
    virtual ~IGameDataMonsterRowRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CollectionSize(
        std::uint32_t address) = 0;
    virtual void LogBind(std::string_view prefix,
                         std::uint32_t table_name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void InitializeStructures(std::uint32_t binding) = 0;
    virtual void BindEffects(std::uint32_t binding) = 0;
    virtual void InitializeRecord(GameDataMonsterRecord& record) = 0;
    virtual void DestroyTemporaryRecordTexts(GameDataMonsterRecord& record) = 0;
    [[nodiscard]] virtual bool ParseBoundRow(
        std::uint32_t binding, std::uint32_t row,
        GameDataMonsterRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t FindKey(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t key) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            const GameDataMonsterRecord& record) = 0;
    virtual void InsertRecord(std::uint32_t array, std::uint32_t end,
                              const GameDataMonsterRecord& record) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t position,
                           std::uint32_t key, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::string DumpRow(std::uint32_t table,
                                              std::uint32_t row) = 0;
};

class IGameDataMonsterAppendRuntime {
public:
    virtual ~IGameDataMonsterAppendRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CollectionSize(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RowSize(std::uint32_t row) = 0;
    virtual void InitializeRecord(GameDataMonsterRecord& record) = 0;
    virtual void DestroyTemporaryRecordTexts(GameDataMonsterRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t PropertyAt(
        std::uint32_t row, std::uint32_t index) = 0;
    virtual void ConvertField(std::uint32_t field,
                              GameDataMonsterRecord& record,
                              std::uint32_t property) = 0;
    virtual void AppendRecord(std::uint32_t array, std::uint32_t end,
                              const GameDataMonsterRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(std::uint32_t table,
                                              std::uint32_t row) = 0;
};

class IGameDataMonsterRefreshRuntime {
public:
    virtual ~IGameDataMonsterRefreshRuntime() = default;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Refresh(std::uint32_t table) = 0;
};

class IGameDataMonsterBindingRuntime {
public:
    virtual ~IGameDataMonsterBindingRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void LogBind(std::string_view prefix,
                         std::uint32_t table_name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void InitializeStructures(std::uint32_t binding) = 0;
    virtual void BindEffects(std::uint32_t binding) = 0;
};

class GameDataMonsterTable final {
public:
    // IDA 0x4dea90: signed-key lookup in 8-byte index records.
    [[nodiscard]] static std::uint32_t FindRecord(
        std::uint32_t manager, std::int32_t key,
        IGameDataMonsterTableRuntime& runtime);
    // IDA 0x4deb00: preload Monster through the Effect-object factory.
    [[nodiscard]] static std::uint8_t Preload(
        std::uint32_t manager, IGameDataMonsterTableRuntime& runtime);
    // IDA 0x4deb20: bind the Monster table or log and throw.
    [[nodiscard]] static std::uint8_t Verify(
        std::uint32_t manager, IGameDataMonsterTableRuntime& runtime);
    // IDA 0x4def50: bind, parse and insert or replace a Monster row.
    [[nodiscard]] static std::uint8_t AddOrReplace(
        std::uint32_t table, std::uint32_t row,
        IGameDataMonsterRowRuntime& runtime);
    // IDA 0x4df230: convert bound fields and append one Monster record.
    static void Append(std::uint32_t table, std::uint32_t row,
                       IGameDataMonsterAppendRuntime& runtime);
    // IDA 0x4df3e0: mark dirty before refreshing the Monster table.
    [[nodiscard]] static std::uint32_t MarkAndRefresh(
        std::uint32_t table, IGameDataMonsterRefreshRuntime& runtime);
    // IDA 0x4dfe90: rebind when pointer and field counts differ.
    static void RefreshBinding(
        std::uint32_t table, IGameDataMonsterBindingRuntime& runtime);
};

}  // namespace pal4
