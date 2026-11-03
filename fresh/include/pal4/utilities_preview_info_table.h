#pragma once

#include "pal4/utilities_record_array_runtime.h"

#include <cstdint>
#include <array>
#include <string>
#include <string_view>

namespace pal4 {

class IUtilitiesPreviewInfoFactoryRuntime {
public:
    virtual ~IUtilitiesPreviewInfoFactoryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t size) = 0;
    virtual void ConstructEffect(std::uint32_t object,
                                 std::uint32_t source) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
};

class IUtilitiesPreviewInfoLifetimeRuntime {
public:
    virtual ~IUtilitiesPreviewInfoLifetimeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void ClearEffect(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t DestructEffect(
        std::uint32_t object) = 0;
    virtual void ReleaseText(std::uint32_t address) = 0;
    virtual void ProcessElements(std::uint32_t address) = 0;
    virtual void DestroyBinding(std::uint32_t address,
                                std::uint32_t flag) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class IUtilitiesPreviewInfoArrayRuntime {
public:
    virtual ~IUtilitiesPreviewInfoArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void DestroyRecord(std::uint32_t record) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class IUtilitiesPreviewInfoClearRuntime {
public:
    virtual ~IUtilitiesPreviewInfoClearRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ClearEffect(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyRecord(
        std::uint32_t record) = 0;
};

using UtilitiesPreviewInfoRecord = std::array<std::uint32_t, 13>;

class IUtilitiesPreviewInfoRowRuntime {
public:
    virtual ~IUtilitiesPreviewInfoRowRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t CollectionSize(
        std::uint32_t collection) = 0;
    virtual void LogBind(std::string_view prefix,
                         std::uint32_t name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void RegisterPreviewFields(std::uint32_t binding) = 0;
    virtual void BindEffects(std::uint32_t binding) = 0;
    virtual void InitializeRecord(UtilitiesPreviewInfoRecord& record) = 0;
    virtual void DestroyTemporaryRecord(
        UtilitiesPreviewInfoRecord& record) = 0;
    [[nodiscard]] virtual bool ParseBoundRow(
        std::uint32_t binding, std::uint32_t row,
        UtilitiesPreviewInfoRecord& record) = 0;
    virtual void InsertRecord(std::uint32_t array, std::uint32_t end,
                              const UtilitiesPreviewInfoRecord& record) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t position,
                           std::uint32_t key, std::uint32_t index) = 0;
    virtual void AssignRecord(std::uint32_t destination,
                              const UtilitiesPreviewInfoRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class IUtilitiesPreviewInfoAppendRuntime {
public:
    virtual ~IUtilitiesPreviewInfoAppendRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RowSize(
        std::uint32_t row) = 0;
    [[nodiscard]] virtual std::uint32_t CollectionSize(
        std::uint32_t collection) = 0;
    virtual void InitializeRecord(UtilitiesPreviewInfoRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupTemporaryRecord(
        UtilitiesPreviewInfoRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t PropertyAt(
        std::uint32_t row, std::uint32_t index) = 0;
    virtual void ConvertField(std::uint32_t binding,
                              UtilitiesPreviewInfoRecord& record,
                              std::uint32_t property) = 0;
    virtual void AppendRecord(std::uint32_t array, std::uint32_t end,
                              const UtilitiesPreviewInfoRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class IUtilitiesPreviewInfoRefreshRuntime {
public:
    virtual ~IUtilitiesPreviewInfoRefreshRuntime() = default;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Refresh(
        std::uint32_t object) = 0;
};

class IUtilitiesPreviewInfoIndexRuntime {
public:
    virtual ~IUtilitiesPreviewInfoIndexRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyRecord(
        std::uint32_t record) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t at,
                           std::uint32_t key, std::uint32_t index) = 0;
};

class IUtilitiesPreviewInfoCapacityRuntime {
public:
    virtual ~IUtilitiesPreviewInfoCapacityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void DestroyRecord(std::uint32_t record) = 0;
};

class UtilitiesPreviewInfoArray final {
public:
    // IDA 0x502520: destroy 52-byte rows and reset the array header.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t array,
        IUtilitiesPreviewInfoArrayRuntime& runtime);
    // IDA 0x502ca0: clean a half-open range of 52-byte records.
    [[nodiscard]] static std::uint32_t DestroyRange(
        std::uint32_t begin, std::uint32_t end,
        IUtilitiesPreviewInfoIndexRuntime& runtime);
    // IDA 0x506580: insert one 52-byte PreviewInfo record.
    [[nodiscard]] static std::uint32_t Insert(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t source, IUtilitiesRecordArrayRuntime& runtime);
    // IDA 0x506770: count active 52-byte records.
    [[nodiscard]] static std::uint32_t Count(
        std::uint32_t array, IUtilitiesRecordArrayRuntime& runtime);
    // IDA 0x5067a0: copy-construct a half-open 52-byte record range.
    [[nodiscard]] static std::uint32_t CopyRange(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination, IUtilitiesRecordArrayRuntime& runtime);
    // IDA 0x507530: insert repeated 52-byte PreviewInfo records.
    static void InsertCopies(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t count, std::uint32_t source,
        IUtilitiesRecordArrayRuntime& runtime);
    // IDA 0x508620: construct repeated 52-byte PreviewInfo records.
    static void Fill(std::uint32_t destination, std::uint32_t count,
                     std::uint32_t source,
                     IUtilitiesRecordArrayRuntime& runtime);
};

class UtilitiesPreviewInfoTable final {
public:
    // IDA 0x502310: factory for the PreviewInfo Effect-derived table.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t source,
        IUtilitiesPreviewInfoFactoryRuntime& runtime);
    // IDA 0x5023e0: release 52-byte records, bindings and base Effect.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object,
        IUtilitiesPreviewInfoLifetimeRuntime& runtime);
    // IDA 0x502560: clear active 52-byte records without freeing storage.
    [[nodiscard]] static std::uint32_t ClearRows(
        std::uint32_t object,
        IUtilitiesPreviewInfoClearRuntime& runtime);
    // IDA 0x5025e0: parse and insert or replace a 52-byte PreviewInfo row.
    [[nodiscard]] static std::uint8_t AddOrReplace(
        std::uint32_t object, std::uint32_t row,
        IUtilitiesPreviewInfoRowRuntime& runtime);
    // IDA 0x5028a0: convert bound fields and append a 52-byte row.
    [[nodiscard]] static std::uint32_t Append(
        std::uint32_t object, std::uint32_t row,
        IUtilitiesPreviewInfoAppendRuntime& runtime);
    // IDA 0x502a50: set the dirty byte before forwarding refresh.
    [[nodiscard]] static std::uint32_t MarkAndRefresh(
        std::uint32_t object,
        IUtilitiesPreviewInfoRefreshRuntime& runtime);
    // IDA 0x502a60: collapse adjacent duplicate rows and rebuild key index.
    static void DeduplicateAndRebuildIndex(
        std::uint32_t object,
        IUtilitiesPreviewInfoIndexRuntime& runtime);
    // IDA 0x502cd0: independently reserve 52-byte rows and 8-byte keys.
    [[nodiscard]] static std::uint32_t Reserve(
        std::uint32_t object, std::int32_t requested,
        IUtilitiesPreviewInfoCapacityRuntime& runtime);
};

}  // namespace pal4
