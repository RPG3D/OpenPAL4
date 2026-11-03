#pragma once

#include <cstdint>
#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace pal4 {

class IGameDataPropertyTableRuntime {
public:
    virtual ~IGameDataPropertyTableRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void LogMissingRecord() = 0;
    [[nodiscard]] virtual int CanStartAction() = 0;
};

class IGameDataPropertyTableLoadRuntime {
public:
    virtual ~IGameDataPropertyTableLoadRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t PreloadTable(
        std::uint32_t database, std::string_view name,
        std::uint32_t factory) = 0;
    [[nodiscard]] virtual std::uint32_t FindTable(
        std::uint32_t database, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t FindProperty(
        std::uint32_t database, std::string_view name) = 0;
    virtual void LogError(std::string_view message) = 0;
};

using GameDataPropertyRecord = std::array<std::uint32_t, 42>;

class IGameDataPropertyRowRuntime {
public:
    virtual ~IGameDataPropertyRowRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CollectionSize(std::uint32_t address) = 0;
    virtual void LogBind(std::string_view prefix, std::uint32_t table_name) = 0;
    virtual void AssignEffects(std::uint32_t destination, std::uint32_t source) = 0;
    virtual void InitializeStructures(std::uint32_t binding) = 0;
    virtual void BindEffects(std::uint32_t binding) = 0;
    virtual void InitializeRecord(GameDataPropertyRecord& record) = 0;
    virtual void DestroyTemporaryRecordTexts(GameDataPropertyRecord& record) = 0;
    [[nodiscard]] virtual bool ParseBoundRow(std::uint32_t binding,
                                              std::uint32_t row,
                                              GameDataPropertyRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t FindKey(std::uint32_t begin,
                                                 std::uint32_t end,
                                                 std::uint32_t key) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            const GameDataPropertyRecord& record) = 0;
    virtual void InsertRecord(std::uint32_t array, std::uint32_t end,
                              const GameDataPropertyRecord& record) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t position,
                           std::uint32_t key, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::string DumpRow(std::uint32_t table,
                                               std::uint32_t row) = 0;
};

class IGameDataPropertyAppendRuntime {
public:
    virtual ~IGameDataPropertyAppendRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CollectionSize(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RowSize(std::uint32_t row) = 0;
    virtual void InitializeRecord(GameDataPropertyRecord& record) = 0;
    virtual void DestroyTemporaryRecordTexts(GameDataPropertyRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t PropertyAt(std::uint32_t row,
                                                    std::uint32_t index) = 0;
    virtual void ConvertField(std::uint32_t field,
                              GameDataPropertyRecord& record,
                              std::uint32_t property) = 0;
    virtual void AppendRecord(std::uint32_t array, std::uint32_t end,
                              const GameDataPropertyRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(std::uint32_t table,
                                               std::uint32_t row) = 0;
};

class IGameDataPropertyRefreshRuntime {
public:
    virtual ~IGameDataPropertyRefreshRuntime() = default;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Refresh(std::uint32_t table) = 0;
};

class IGameDataPropertyBindingRuntime {
public:
    virtual ~IGameDataPropertyBindingRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void LogBind(std::string_view prefix,
                         std::uint32_t table_name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void InitializeStructures(std::uint32_t binding) = 0;
    virtual void BindEffects(std::uint32_t binding) = 0;
};

struct GameDataPropertyKeyIndex {
    std::int32_t key;
    std::uint32_t record_index;
    bool operator==(const GameDataPropertyKeyIndex&) const = default;
};

class IGameDataPropertyIndexRuntime {
public:
    virtual ~IGameDataPropertyIndexRuntime() = default;
    virtual void AssignRecord(GameDataPropertyRecord& destination,
                              const GameDataPropertyRecord& source) = 0;
    virtual void DestroyRecord(GameDataPropertyRecord& record) = 0;
};

class GameDataPropertyIndex final {
public:
    // IDA 0x4e5f90: compact adjacent duplicate keys, then rebuild and sort.
    static void Rebuild(std::vector<GameDataPropertyRecord>& records,
                        std::vector<GameDataPropertyKeyIndex>& index,
                        IGameDataPropertyIndexRuntime& runtime);
};

class GameDataPropertyTable final {
public:
    // IDA 0x4e54d0: bound three signed record categories as nonnegative.
    [[nodiscard]] static bool IsValidCategory(
        std::uint32_t record, IGameDataPropertyTableRuntime& runtime);
    // IDA 0x4e5500: signed AI command value must be less than six.
    [[nodiscard]] static bool IsAiCommandBelowSix(
        std::uint32_t record, IGameDataPropertyTableRuntime& runtime);
    // IDA 0x4e5510: selected IDs require the actor to allow an action.
    [[nodiscard]] static bool CanUseRecord(
        std::uint32_t record, IGameDataPropertyTableRuntime& runtime);
    // IDA 0x4e5590: find a record by signed key, logging an absent result.
    [[nodiscard]] static std::uint32_t FindOrLog(
        std::uint32_t table, std::int32_t key,
        IGameDataPropertyTableRuntime& runtime);
    // IDA 0x4e5620: check the same index without logging.
    [[nodiscard]] static bool Contains(
        std::uint32_t table, std::int32_t key,
        IGameDataPropertyTableRuntime& runtime);
    // IDA 0x4e56a0: preload Property through its Effect factory.
    [[nodiscard]] static std::uint8_t Preload(
        std::uint32_t manager, IGameDataPropertyTableLoadRuntime& runtime);
    // IDA 0x4e56c0: verify Property and PropertyClass or throw.
    [[nodiscard]] static std::uint8_t Verify(
        std::uint32_t manager, IGameDataPropertyTableLoadRuntime& runtime);
    // IDA 0x4e5b20: bind fields, parse and insert or replace a Property row.
    [[nodiscard]] static std::uint8_t AddOrReplace(
        std::uint32_t table, std::uint32_t row,
        IGameDataPropertyRowRuntime& runtime);
    // IDA 0x4e5e00: convert each bound field and append a Property record.
    static void Append(std::uint32_t table, std::uint32_t row,
                       IGameDataPropertyAppendRuntime& runtime);
    // IDA 0x4e5f80: mark dirty before refreshing the index.
    [[nodiscard]] static std::uint32_t MarkAndRefresh(
        std::uint32_t table, IGameDataPropertyRefreshRuntime& runtime);
    // IDA 0x4e6a40: rebind when field and bound-pointer counts differ.
    static void RefreshBinding(
        std::uint32_t table, IGameDataPropertyBindingRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t Find(
        std::uint32_t table, std::int32_t key,
        IGameDataPropertyTableRuntime& runtime);
};

}  // namespace pal4
