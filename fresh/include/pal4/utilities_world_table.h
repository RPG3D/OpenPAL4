#pragma once

#include "pal4/utilities_record_array_runtime.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IUtilitiesWorldFactoryRuntime {
public:
    virtual ~IUtilitiesWorldFactoryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void ConstructEffect(std::uint32_t object,
                                 std::uint32_t source) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
};

class IUtilitiesWorldLifetimeRuntime {
public:
    virtual ~IUtilitiesWorldLifetimeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void ClearEffect(std::uint32_t object) = 0;
    virtual void ReleaseText(std::uint32_t address) = 0;
    virtual void ProcessElements(std::uint32_t address) = 0;
    virtual void DestroyBinding(std::uint32_t address,
                                std::uint32_t flag) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestructEffect(
        std::uint32_t object) = 0;
};

class IUtilitiesWorldArrayRuntime {
public:
    virtual ~IUtilitiesWorldArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void DestroyRecord(std::uint32_t record) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class IUtilitiesWorldClearRuntime {
public:
    virtual ~IUtilitiesWorldClearRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ClearEffect(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyRecord(
        std::uint32_t record) = 0;
};

using UtilitiesWorldRecord = std::array<std::uint32_t, 10>;

class IUtilitiesWorldRowRuntime {
public:
    virtual ~IUtilitiesWorldRowRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t CollectionSize(
        std::uint32_t collection) = 0;
    virtual void LogBind(std::string_view prefix,
                         std::uint32_t name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void RegisterWorldFields(std::uint32_t binding) = 0;
    virtual void BindEffects(std::uint32_t binding) = 0;
    virtual void InitializeRecord(UtilitiesWorldRecord& record) = 0;
    virtual void DestroyTemporaryRecord(
        UtilitiesWorldRecord& record) = 0;
    [[nodiscard]] virtual bool ParseBoundRow(
        std::uint32_t binding, std::uint32_t row,
        UtilitiesWorldRecord& record) = 0;
    virtual void InsertRecord(std::uint32_t array, std::uint32_t end,
                              const UtilitiesWorldRecord& record) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t position,
                           std::uint32_t key, std::uint32_t index) = 0;
    virtual void AssignRecord(std::uint32_t destination,
                              const UtilitiesWorldRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class IUtilitiesWorldAppendRuntime {
public:
    virtual ~IUtilitiesWorldAppendRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RowSize(
        std::uint32_t row) = 0;
    [[nodiscard]] virtual std::uint32_t CollectionSize(
        std::uint32_t collection) = 0;
    virtual void InitializeRecord(UtilitiesWorldRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupTemporaryRecord(
        UtilitiesWorldRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t PropertyAt(
        std::uint32_t row, std::uint32_t index) = 0;
    virtual void ConvertField(std::uint32_t binding,
                              UtilitiesWorldRecord& record,
                              std::uint32_t property) = 0;
    virtual void AppendRecord(std::uint32_t array, std::uint32_t end,
                              const UtilitiesWorldRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class IUtilitiesWorldRefreshRuntime {
public:
    virtual ~IUtilitiesWorldRefreshRuntime() = default;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Refresh(
        std::uint32_t object) = 0;
};

class IUtilitiesWorldIndexRuntime {
public:
    virtual ~IUtilitiesWorldIndexRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyRecord(
        std::uint32_t record) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t at,
                           std::uint32_t key, std::uint32_t index) = 0;
};

class IUtilitiesWorldCapacityRuntime {
public:
    virtual ~IUtilitiesWorldCapacityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void DestroyRecord(std::uint32_t record) = 0;
};

class UtilitiesWorldArray final {
public:
    // IDA 0x505880: destroy 40-byte rows and reset the array header.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t array, IUtilitiesWorldArrayRuntime& runtime);
    // IDA 0x505fa0: destroy a half-open range of 40-byte rows.
    [[nodiscard]] static std::uint32_t DestroyRange(
        std::uint32_t begin, std::uint32_t end,
        IUtilitiesWorldIndexRuntime& runtime);
    // IDA 0x507000: insert one 40-byte World record.
    [[nodiscard]] static std::uint32_t Insert(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t source, IUtilitiesRecordArrayRuntime& runtime);
    // IDA 0x5071e0: copy-construct a half-open 40-byte record range.
    [[nodiscard]] static std::uint32_t CopyRange(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination, IUtilitiesRecordArrayRuntime& runtime);
    // IDA 0x508280: insert repeated 40-byte World records.
    static void InsertCopies(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t count, std::uint32_t source,
        IUtilitiesRecordArrayRuntime& runtime);
    // IDA 0x5086e0: construct repeated 40-byte World records.
    static void Fill(std::uint32_t destination, std::uint32_t count,
                     std::uint32_t source,
                     IUtilitiesRecordArrayRuntime& runtime);
};

class UtilitiesWorldTable final {
public:
    // IDA 0x505670: create the World Effect-derived table.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t source, IUtilitiesWorldFactoryRuntime& runtime);
    // IDA 0x505740: release row texts, bindings and three buffers.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IUtilitiesWorldLifetimeRuntime& runtime);
    // IDA 0x5058c0: clear active rows while retaining capacity.
    [[nodiscard]] static std::uint32_t ClearRows(
        std::uint32_t object, IUtilitiesWorldClearRuntime& runtime);
    // IDA 0x505930: parse and insert or replace a 40-byte row.
    [[nodiscard]] static std::uint8_t AddOrReplace(
        std::uint32_t object, std::uint32_t row,
        IUtilitiesWorldRowRuntime& runtime);
    // IDA 0x505bb0: convert bound fields and append a 40-byte row.
    [[nodiscard]] static std::uint32_t Append(
        std::uint32_t object, std::uint32_t row,
        IUtilitiesWorldAppendRuntime& runtime);
    // IDA 0x505d50: mark the World table dirty and forward refresh.
    [[nodiscard]] static std::uint32_t MarkAndRefresh(
        std::uint32_t object,
        IUtilitiesWorldRefreshRuntime& runtime);
    // IDA 0x505d60: compact adjacent equal keys and rebuild signed index.
    static void DeduplicateAndRebuildIndex(
        std::uint32_t object, IUtilitiesWorldIndexRuntime& runtime);
    // IDA 0x505fd0: reserve rows and index entries independently.
    [[nodiscard]] static std::uint32_t Reserve(
        std::uint32_t object, std::int32_t requested,
        IUtilitiesWorldCapacityRuntime& runtime);
};

}  // namespace pal4
