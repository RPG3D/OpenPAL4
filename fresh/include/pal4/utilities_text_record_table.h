#pragma once

#include "pal4/utilities_record_array_runtime.h"

#include <cstdint>
#include <array>
#include <string>
#include <string_view>

namespace pal4 {

class IUtilitiesTextTableRuntime {
public:
    virtual ~IUtilitiesTextTableRuntime() = default;
    virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void ConstructEffect(std::uint32_t object,
                                 std::uint32_t source) = 0;
    virtual std::uint32_t DestructEffect(std::uint32_t object) = 0;
    virtual std::uint32_t ClearEffectData(std::uint32_t object) = 0;
    virtual void ProcessElements(std::uint32_t object) = 0;
    virtual void ReleaseRecordText(std::uint32_t text) = 0;
    virtual std::uint32_t CleanupProperties5(std::uint32_t record) = 0;
    virtual void DestroyBindingElement(std::uint32_t record,
                                       std::uint32_t flag) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
};

using UtilitiesTextRecord = std::array<std::uint32_t, 5>;

class IUtilitiesTextRowRuntime {
public:
    virtual ~IUtilitiesTextRowRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t CollectionSize(
        std::uint32_t collection) = 0;
    virtual void LogBind(std::string_view prefix,
                         std::uint32_t name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void RegisterIdString(std::uint32_t binding) = 0;
    virtual void BindEffects(std::uint32_t binding) = 0;
    virtual void InitializeRecord(UtilitiesTextRecord& record) = 0;
    virtual void DestroyTemporaryRecord(UtilitiesTextRecord& record) = 0;
    [[nodiscard]] virtual bool ParseBoundRow(
        std::uint32_t binding, std::uint32_t row,
        UtilitiesTextRecord& record) = 0;
    virtual void InsertRecord(std::uint32_t array, std::uint32_t end,
                              const UtilitiesTextRecord& record) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t position,
                           std::uint32_t key, std::uint32_t index) = 0;
    virtual void AssignRecord(std::uint32_t destination,
                              const UtilitiesTextRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class IUtilitiesTextAppendRuntime {
public:
    virtual ~IUtilitiesTextAppendRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RowSize(std::uint32_t row) = 0;
    [[nodiscard]] virtual std::uint32_t CollectionSize(
        std::uint32_t collection) = 0;
    virtual void InitializeRecord(UtilitiesTextRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupTemporaryRecord(
        UtilitiesTextRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t PropertyAt(
        std::uint32_t row, std::uint32_t index) = 0;
    virtual void ConvertField(std::uint32_t binding,
                              UtilitiesTextRecord& record,
                              std::uint32_t property) = 0;
    virtual void AppendRecord(std::uint32_t array, std::uint32_t end,
                              const UtilitiesTextRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class IUtilitiesTextRefreshRuntime {
public:
    virtual ~IUtilitiesTextRefreshRuntime() = default;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Refresh(std::uint32_t object) = 0;
};

class IUtilitiesTextIndexRuntime {
public:
    virtual ~IUtilitiesTextIndexRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyRecord(
        std::uint32_t record) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t at,
                           std::uint32_t key, std::uint32_t index) = 0;
};

class IUtilitiesTextCapacityRuntime {
public:
    virtual ~IUtilitiesTextCapacityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void DestroyRecord(std::uint32_t record) = 0;
};

class UtilitiesTextRecordArray final {
public:
    // IDA 0x501a80: destroy each 20-byte property row and free its array.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t array, IUtilitiesTextTableRuntime& runtime);
    // IDA 0x502170: clean a half-open range of 20-byte records.
    [[nodiscard]] static std::uint32_t DestroyRange(
        std::uint32_t begin, std::uint32_t end,
        IUtilitiesTextIndexRuntime& runtime);
    // IDA 0x506330: insert one 20-byte text record.
    [[nodiscard]] static std::uint32_t Insert(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t source, IUtilitiesRecordArrayRuntime& runtime);
    // IDA 0x506510: count active 20-byte records.
    [[nodiscard]] static std::uint32_t Count(
        std::uint32_t array, IUtilitiesRecordArrayRuntime& runtime);
    // IDA 0x506540: copy-construct a half-open 20-byte record range.
    [[nodiscard]] static std::uint32_t CopyRange(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination, IUtilitiesRecordArrayRuntime& runtime);
    // IDA 0x507220: insert repeated 20-byte text records.
    static void InsertCopies(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t count, std::uint32_t source,
        IUtilitiesRecordArrayRuntime& runtime);
    // IDA 0x5085f0: construct repeated 20-byte text records.
    static void Fill(std::uint32_t destination, std::uint32_t count,
                     std::uint32_t source,
                     IUtilitiesRecordArrayRuntime& runtime);
};

class UtilitiesTextRecordTable final {
public:
    // IDA 0x501870: shared factory for GameString and Tutorial tables.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t source, IUtilitiesTextTableRuntime& runtime);
    // IDA 0x501940: release three arrays before the Effect base object.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IUtilitiesTextTableRuntime& runtime);
    // IDA 0x501ac0: clear active records without freeing their storage.
    [[nodiscard]] static std::uint32_t ClearRows(
        std::uint32_t object, IUtilitiesTextTableRuntime& runtime);
    // IDA 0x501b20: bind, parse and insert or replace a 20-byte row.
    [[nodiscard]] static std::uint8_t AddOrReplace(
        std::uint32_t object, std::uint32_t row,
        IUtilitiesTextRowRuntime& runtime);
    // IDA 0x501db0: convert bound fields and append a 20-byte row.
    [[nodiscard]] static std::uint32_t Append(
        std::uint32_t object, std::uint32_t row,
        IUtilitiesTextAppendRuntime& runtime);
    // IDA 0x501f20: set dirty flag before forwarding refresh.
    [[nodiscard]] static std::uint32_t MarkAndRefresh(
        std::uint32_t object, IUtilitiesTextRefreshRuntime& runtime);
    // IDA 0x501f30: collapse adjacent equal keys and rebuild sorted index.
    static void DeduplicateAndRebuildIndex(
        std::uint32_t object, IUtilitiesTextIndexRuntime& runtime);
    // IDA 0x5021a0: independently reserve 20-byte rows and 8-byte keys.
    [[nodiscard]] static std::uint32_t Reserve(
        std::uint32_t object, std::int32_t requested,
        IUtilitiesTextCapacityRuntime& runtime);
};

}  // namespace pal4
