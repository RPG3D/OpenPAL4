#pragma once

#include "pal4/game_data_table_binding_refresh.h"

#include <cstdint>
#include <array>
#include <string>
#include <string_view>

namespace pal4 {

class IStuntDataTableRuntime {
public:
    virtual ~IStuntDataTableRuntime() = default;
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
    [[nodiscard]] virtual std::uint32_t DestroyStuntRecord(
        std::uint32_t record) = 0;
    virtual void DestroyPropertyRecord(std::uint32_t record) = 0;
    virtual void ProcessElements(std::uint32_t address) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyBase(
        std::uint32_t object) = 0;
};

using StuntDataRecord = std::array<std::uint32_t, 52>;

class IStuntDataRowRuntime {
public:
    virtual ~IStuntDataRowRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t CollectionSize(
        std::uint32_t collection) = 0;
    virtual void LogBind(std::string_view prefix,
                         std::uint32_t name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void ProcessPropertyRegistry(std::uint32_t binding) = 0;
    virtual void BindEffects(std::uint32_t binding) = 0;
    virtual void InitializeRecord(StuntDataRecord& record) = 0;
    [[nodiscard]] virtual std::uint8_t CleanupTemporaryRecord(
        StuntDataRecord& record) = 0;
    [[nodiscard]] virtual bool ParseBoundRow(
        std::uint32_t binding, std::uint32_t row,
        StuntDataRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t FindKey(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t key) = 0;
    virtual void InsertRecord(std::uint32_t array, std::uint32_t end,
                              const StuntDataRecord& record) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t position,
                           std::uint32_t key, std::uint32_t index) = 0;
    virtual void AssignRecord(std::uint32_t destination,
                              const StuntDataRecord& record) = 0;
    virtual void AppendRecord(std::uint32_t array, std::uint32_t end,
                              const StuntDataRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class IStuntDataRefreshRuntime {
public:
    virtual ~IStuntDataRefreshRuntime() = default;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Refresh(std::uint32_t table) = 0;
};

class IStuntDataIndexRuntime {
public:
    virtual ~IStuntDataIndexRuntime() = default;
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

class IStuntDataCapacityRuntime {
public:
    virtual ~IStuntDataCapacityRuntime() = default;
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

class StuntDataTable final {
public:
    static constexpr std::uint32_t kObjectBytes = 140;
    static constexpr std::uint32_t kVtable = 0x84470c;
    static constexpr std::uint32_t kRecordBytes = 208;
    static constexpr std::uint32_t kPropertyRecordBytes = 60;

    // IDA 0x4f49c0: allocate and initialize an Effect-derived Stunt table.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t name, IStuntDataTableRuntime& runtime);
    // IDA 0x4f4a90: release records, bindings and the Effect base.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IStuntDataTableRuntime& runtime);
    // IDA 0x4f4bd0: destroy a standalone Stunt record array.
    [[nodiscard]] static std::uint32_t ReleaseRecordArray(
        std::uint32_t array, IStuntDataTableRuntime& runtime);
    // IDA 0x4f4c10: clear active Stunt rows while retaining capacity.
    [[nodiscard]] static std::uint32_t ClearRows(
        std::uint32_t object, IStuntDataTableRuntime& runtime);
    // IDA 0x4f4c70: bind, parse, validate and insert or replace a Stunt row.
    [[nodiscard]] static std::uint8_t AddOrReplace(
        std::uint32_t table, std::uint32_t row,
        IStuntDataRowRuntime& runtime);
    // IDA 0x4f4f90: parse and append a 208-byte Stunt record.
    [[nodiscard]] static std::uint8_t Append(
        std::uint32_t table, std::uint32_t row,
        IStuntDataRowRuntime& runtime);
    // IDA 0x4f5190: mark the Stunt table dirty and refresh it.
    [[nodiscard]] static std::uint32_t MarkAndRefresh(
        std::uint32_t table, IStuntDataRefreshRuntime& runtime);
    // IDA 0x4f51a0: compact adjacent duplicate IDs and rebuild signed index.
    static void RebuildIndex(std::uint32_t table,
                             IStuntDataIndexRuntime& runtime);
    // IDA 0x4f53e0: destroy a half-open range of 208-byte Stunt records.
    [[nodiscard]] static std::uint32_t DestroyRange(
        std::uint32_t begin, std::uint32_t end,
        IStuntDataIndexRuntime& runtime);
    // IDA 0x4f5410: reserve Stunt records and key/index pairs independently.
    [[nodiscard]] static std::uint32_t Reserve(
        std::uint32_t table, std::int32_t requested,
        IStuntDataCapacityRuntime& runtime);
    // IDA 0x4f7910: refresh Stunt property bindings on a count mismatch.
    [[nodiscard]] static std::uint32_t RefreshBindings(
        std::uint32_t table, IGameDataTableBindingRefreshRuntime& runtime);
};

}  // namespace pal4
