#pragma once

#include "pal4/game_data_table_binding_refresh.h"

#include <cstdint>
#include <array>
#include <string>
#include <string_view>

namespace pal4 {

class IMagicDataTableRuntime {
public:
    virtual ~IMagicDataTableRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void ConstructBase(std::uint32_t object,
                               std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ClearEffectData(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyMagicRecord(
        std::uint32_t record) = 0;
    virtual void DestroyPropertyRecord(std::uint32_t record) = 0;
    virtual void ProcessElements(std::uint32_t address) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyBase(
        std::uint32_t object) = 0;
};

using MagicDataRecord = std::array<std::uint32_t, 38>;

class IMagicDataCountRuntime {
public:
    virtual ~IMagicDataCountRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
};

class IMagicDataRowRuntime : public IMagicDataCountRuntime {
public:
    [[nodiscard]] virtual std::int32_t CollectionSize(
        std::uint32_t collection) = 0;
    virtual void LogBind(std::string_view prefix,
                         std::uint32_t name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void ProcessPropertyRegistry(std::uint32_t binding) = 0;
    virtual void BindEffects(std::uint32_t binding) = 0;
    virtual void InitializeRecord(MagicDataRecord& record) = 0;
    [[nodiscard]] virtual std::uint8_t CleanupTemporaryRecord(
        MagicDataRecord& record) = 0;
    [[nodiscard]] virtual bool ParseBoundRow(
        std::uint32_t binding, std::uint32_t row,
        MagicDataRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t FindKey(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t key) = 0;
    virtual void InsertRecord(std::uint32_t array, std::uint32_t end,
                              const MagicDataRecord& record) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t position,
                           std::uint32_t key, std::uint32_t index) = 0;
    virtual void AssignRecord(std::uint32_t destination,
                              const MagicDataRecord& record) = 0;
    virtual void AppendRecord(std::uint32_t array, std::uint32_t end,
                              const MagicDataRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class IMagicDataRefreshRuntime {
public:
    virtual ~IMagicDataRefreshRuntime() = default;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Refresh(std::uint32_t table) = 0;
};

class IMagicDataIndexRuntime {
public:
    virtual ~IMagicDataIndexRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void AssignRecord(std::uint32_t destination,
                              std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyRecord(
        std::uint32_t record) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t end,
                           std::uint32_t key, std::uint32_t index) = 0;
};

class IMagicDataCapacityRuntime {
public:
    virtual ~IMagicDataCapacityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void DestroyRecord(std::uint32_t record) = 0;
    virtual void CopyIndex(std::uint32_t destination,
                           std::uint32_t source) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class MagicDataTable final {
public:
    static constexpr std::uint32_t kObjectBytes = 140;
    static constexpr std::uint32_t kVtable = 0x84472c;
    static constexpr std::uint32_t kRecordBytes = 152;
    static constexpr std::uint32_t kPropertyRecordBytes = 60;

    // IDA 0x4f55a0: allocate and initialize an Effect-derived Magic table.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t name, IMagicDataTableRuntime& runtime);
    // IDA 0x4f5670: release records, bindings and the Effect base.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IMagicDataTableRuntime& runtime);
    // IDA 0x4f57b0: destroy a standalone Magic record array.
    [[nodiscard]] static std::uint32_t ReleaseRecordArray(
        std::uint32_t array, IMagicDataTableRuntime& runtime);
    // IDA 0x4f57f0: clear active Magic rows while retaining capacity.
    [[nodiscard]] static std::uint32_t ClearRows(
        std::uint32_t object, IMagicDataTableRuntime& runtime);
    // IDA 0x4f5900: bind, parse, validate and insert or replace a Magic row.
    [[nodiscard]] static std::uint8_t AddOrReplace(
        std::uint32_t table, std::uint32_t row,
        IMagicDataRowRuntime& runtime);
    // IDA 0x4f5c20: count active 152-byte records in an array.
    [[nodiscard]] static std::int32_t RecordCount(
        std::uint32_t array, IMagicDataCountRuntime& runtime);
    // IDA 0x4f5c50: parse and append a 152-byte Magic record.
    [[nodiscard]] static std::uint8_t Append(
        std::uint32_t table, std::uint32_t row,
        IMagicDataRowRuntime& runtime);
    // IDA 0x4f5e50: mark the Magic table dirty and refresh it.
    [[nodiscard]] static std::uint32_t MarkAndRefresh(
        std::uint32_t table, IMagicDataRefreshRuntime& runtime);
    // IDA 0x4f5e60: compact adjacent duplicate IDs and rebuild signed index.
    static void RebuildIndex(std::uint32_t table,
                             IMagicDataIndexRuntime& runtime);
    // IDA 0x4f60a0: destroy a half-open range of 152-byte Magic records.
    [[nodiscard]] static std::uint32_t DestroyRange(
        std::uint32_t begin, std::uint32_t end,
        IMagicDataIndexRuntime& runtime);
    // IDA 0x4f60d0: reserve Magic records and key/index pairs independently.
    [[nodiscard]] static std::uint32_t Reserve(
        std::uint32_t table, std::int32_t requested,
        IMagicDataCapacityRuntime& runtime);
    // IDA 0x4f7b40: refresh Magic property bindings on a count mismatch.
    [[nodiscard]] static std::uint32_t RefreshBindings(
        std::uint32_t table, IGameDataTableBindingRefreshRuntime& runtime);
};

}  // namespace pal4
