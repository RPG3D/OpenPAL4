#pragma once

#include "pal4/utilities_record_array_runtime.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IUtilitiesWindowFactoryRuntime {
public:
    virtual ~IUtilitiesWindowFactoryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void ConstructEffect(std::uint32_t object,
                                 std::uint32_t source) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
};

class IUtilitiesWindowLifetimeRuntime {
public:
    virtual ~IUtilitiesWindowLifetimeRuntime() = default;
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

class IUtilitiesWindowArrayRuntime {
public:
    virtual ~IUtilitiesWindowArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void DestroyRecord(std::uint32_t record) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class IUtilitiesWindowClearRuntime {
public:
    virtual ~IUtilitiesWindowClearRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ClearEffect(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyRecord(
        std::uint32_t record) = 0;
};

using UtilitiesWindowRecord = std::array<std::uint32_t, 11>;

class IUtilitiesWindowRowRuntime {
public:
    virtual ~IUtilitiesWindowRowRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t CollectionSize(
        std::uint32_t collection) = 0;
    virtual void LogBind(std::string_view prefix,
                         std::uint32_t name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void RegisterWindowFields(std::uint32_t binding) = 0;
    virtual void BindEffects(std::uint32_t binding) = 0;
    virtual void InitializeRecord(UtilitiesWindowRecord& record) = 0;
    virtual void DestroyTemporaryRecord(
        UtilitiesWindowRecord& record) = 0;
    [[nodiscard]] virtual bool ParseBoundRow(
        std::uint32_t binding, std::uint32_t row,
        UtilitiesWindowRecord& record) = 0;
    virtual void InsertRecord(std::uint32_t array, std::uint32_t end,
                              const UtilitiesWindowRecord& record) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t position,
                           std::uint32_t key, std::uint32_t index) = 0;
    virtual void AssignRecord(std::uint32_t destination,
                              const UtilitiesWindowRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class IUtilitiesWindowAppendRuntime {
public:
    virtual ~IUtilitiesWindowAppendRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RowSize(
        std::uint32_t row) = 0;
    [[nodiscard]] virtual std::uint32_t CollectionSize(
        std::uint32_t collection) = 0;
    virtual void InitializeRecord(UtilitiesWindowRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupTemporaryRecord(
        UtilitiesWindowRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t PropertyAt(
        std::uint32_t row, std::uint32_t index) = 0;
    virtual void ConvertField(std::uint32_t binding,
                              UtilitiesWindowRecord& record,
                              std::uint32_t property) = 0;
    virtual void AppendRecord(std::uint32_t array, std::uint32_t end,
                              const UtilitiesWindowRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class IUtilitiesWindowRefreshRuntime {
public:
    virtual ~IUtilitiesWindowRefreshRuntime() = default;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Refresh(
        std::uint32_t object) = 0;
};

class IUtilitiesWindowIndexRuntime {
public:
    virtual ~IUtilitiesWindowIndexRuntime() = default;
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

class IUtilitiesWindowCapacityRuntime {
public:
    virtual ~IUtilitiesWindowCapacityRuntime() = default;
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

class UtilitiesWindowArray final {
public:
    // IDA 0x504430: destroy 44-byte rows and reset the array header.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t array, IUtilitiesWindowArrayRuntime& runtime);
    // IDA 0x504b50: clean a half-open range of 44-byte rows.
    [[nodiscard]] static std::uint32_t DestroyRange(
        std::uint32_t begin, std::uint32_t end,
        IUtilitiesWindowIndexRuntime& runtime);
    // IDA 0x506c30: insert one 44-byte Window record.
    [[nodiscard]] static std::uint32_t Insert(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t source, IUtilitiesRecordArrayRuntime& runtime);
    // IDA 0x506e20: copy-construct a half-open 44-byte record range.
    [[nodiscard]] static std::uint32_t CopyRange(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination, IUtilitiesRecordArrayRuntime& runtime);
    // IDA 0x507f40: insert repeated 44-byte Window records.
    static void InsertCopies(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t count, std::uint32_t source,
        IUtilitiesRecordArrayRuntime& runtime);
    // IDA 0x5086b0: construct repeated 44-byte Window records.
    static void Fill(std::uint32_t destination, std::uint32_t count,
                     std::uint32_t source,
                     IUtilitiesRecordArrayRuntime& runtime);
};

class UtilitiesWindowTable final {
public:
    // IDA 0x504220: create the Window Effect-derived table.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t source, IUtilitiesWindowFactoryRuntime& runtime);
    // IDA 0x5042f0: release row texts, bindings and three buffers.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IUtilitiesWindowLifetimeRuntime& runtime);
    // IDA 0x504470: clear active rows while retaining capacity.
    [[nodiscard]] static std::uint32_t ClearRows(
        std::uint32_t object, IUtilitiesWindowClearRuntime& runtime);
    // IDA 0x5044e0: parse and insert or replace a 44-byte row.
    [[nodiscard]] static std::uint8_t AddOrReplace(
        std::uint32_t object, std::uint32_t row,
        IUtilitiesWindowRowRuntime& runtime);
    // IDA 0x504760: convert bound fields and append a row.
    [[nodiscard]] static std::uint32_t Append(
        std::uint32_t object, std::uint32_t row,
        IUtilitiesWindowAppendRuntime& runtime);
    // IDA 0x504900: mark the Window table dirty and forward refresh.
    [[nodiscard]] static std::uint32_t MarkAndRefresh(
        std::uint32_t object,
        IUtilitiesWindowRefreshRuntime& runtime);
    // IDA 0x504910: compact adjacent duplicates and rebuild key index.
    static void DeduplicateAndRebuildIndex(
        std::uint32_t object,
        IUtilitiesWindowIndexRuntime& runtime);
    // IDA 0x504b80: independently reserve 44-byte rows and 8-byte keys.
    [[nodiscard]] static std::uint32_t Reserve(
        std::uint32_t object, std::int32_t requested,
        IUtilitiesWindowCapacityRuntime& runtime);
};

}  // namespace pal4
