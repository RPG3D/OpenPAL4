#pragma once

#include <cstdint>
#include <array>
#include <string>
#include <string_view>

namespace pal4 {

class IGameDataParameterTableRuntime {
public:
    virtual ~IGameDataParameterTableRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void LogMissingRecord() = 0;
};

class IGameDataParameterTableLoadRuntime {
public:
    virtual ~IGameDataParameterTableLoadRuntime() = default;
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

using GameDataParameterRecord = std::array<std::uint32_t, 82>;

class IGameDataParameterRowRuntime {
public:
    virtual ~IGameDataParameterRowRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CollectionSize(
        std::uint32_t address) = 0;
    virtual void LogBind(std::string_view prefix,
                         std::uint32_t table_name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void InitializeStructures(std::uint32_t binding) = 0;
    virtual void BindEffects(std::uint32_t binding) = 0;
    virtual void InitializeRecord(GameDataParameterRecord& record) = 0;
    virtual void DestroyTemporaryRecordTexts(
        GameDataParameterRecord& record) = 0;
    [[nodiscard]] virtual bool ParseBoundRow(
        std::uint32_t binding, std::uint32_t row,
        GameDataParameterRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t FindKey(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t key) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            const GameDataParameterRecord& record) = 0;
    virtual void InsertRecord(std::uint32_t array, std::uint32_t end,
                              const GameDataParameterRecord& record) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t position,
                           std::uint32_t key, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class IGameDataParameterAppendRuntime {
public:
    virtual ~IGameDataParameterAppendRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CollectionSize(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RowSize(std::uint32_t row) = 0;
    virtual void InitializeRecord(GameDataParameterRecord& record) = 0;
    virtual void DestroyTemporaryRecordTexts(
        GameDataParameterRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t PropertyAt(
        std::uint32_t row, std::uint32_t index) = 0;
    virtual void ConvertField(std::uint32_t field,
                              GameDataParameterRecord& record,
                              std::uint32_t property) = 0;
    virtual void AppendRecord(std::uint32_t array, std::uint32_t end,
                              const GameDataParameterRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class IGameDataParameterRefreshRuntime {
public:
    virtual ~IGameDataParameterRefreshRuntime() = default;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Refresh(std::uint32_t table) = 0;
};

class IGameDataParameterBindingRuntime {
public:
    virtual ~IGameDataParameterBindingRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void LogBind(std::string_view prefix,
                         std::uint32_t table_name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void InitializeStructures(std::uint32_t binding) = 0;
    virtual void BindEffects(std::uint32_t binding) = 0;
};

class GameDataParameterTable final {
public:
    // IDA 0x4e2990: preload Prescription with its Effect-object factory.
    [[nodiscard]] static std::uint8_t Preload(
        std::uint32_t manager, IGameDataParameterTableLoadRuntime& runtime);
    // IDA 0x4e29b0: verify Prescription and PrescriptionType or throw.
    [[nodiscard]] static std::uint8_t Verify(
        std::uint32_t manager, IGameDataParameterTableLoadRuntime& runtime);
    // IDA 0x4e2830: the record's +100 value must be 0, 1 or 2.
    [[nodiscard]] static bool IsValidCategory(
        std::uint32_t record, IGameDataParameterTableRuntime& runtime);
    // IDA 0x4e2880: find a record by signed key; log an absent record.
    [[nodiscard]] static std::uint32_t FindOrLog(
        std::uint32_t table, std::int32_t key,
        IGameDataParameterTableRuntime& runtime);
    // IDA 0x4e2910: check the same index without logging.
    [[nodiscard]] static bool Contains(
        std::uint32_t table, std::int32_t key,
        IGameDataParameterTableRuntime& runtime);
    // IDA 0x4e2d90: bind, parse and insert or replace a parameter row.
    [[nodiscard]] static std::uint8_t AddOrReplace(
        std::uint32_t table, std::uint32_t row,
        IGameDataParameterRowRuntime& runtime);
    // IDA 0x4e3070: convert the bound fields and append a parameter record.
    static void Append(std::uint32_t table, std::uint32_t row,
                       IGameDataParameterAppendRuntime& runtime);
    // IDA 0x4e31f0: mark dirty before table refresh.
    [[nodiscard]] static std::uint32_t MarkAndRefresh(
        std::uint32_t table, IGameDataParameterRefreshRuntime& runtime);
    // IDA 0x4e3bc0: rebind when field and bound-pointer counts differ.
    static void RefreshBinding(
        std::uint32_t table, IGameDataParameterBindingRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t Find(
        std::uint32_t table, std::int32_t key,
        IGameDataParameterTableRuntime& runtime);
};

}  // namespace pal4
