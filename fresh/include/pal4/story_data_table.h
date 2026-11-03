#pragma once

#include "pal4/game_data_table_binding_refresh.h"

#include <cstdint>
#include <array>
#include <string>
#include <string_view>

namespace pal4 {

class IStoryDataTableRuntime {
public:
    virtual ~IStoryDataTableRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void ConstructBase(std::uint32_t object,
                               std::uint32_t name) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class IStoryDataTableDestroyRuntime {
public:
    virtual ~IStoryDataTableDestroyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void ClearEffectData(std::uint32_t table) = 0;
    virtual void DestroyStoryText(std::uint32_t text) = 0;
    virtual void ProcessElements(std::uint32_t elements) = 0;
    virtual void DestroyPropertyRecord(std::uint32_t record) = 0;
    virtual void DestroyStoryRecord(std::uint32_t record) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyBase(
        std::uint32_t table) = 0;
};

using StoryDataRecord = std::array<std::uint32_t, 9>;

class IStoryDataRowRuntime {
public:
    virtual ~IStoryDataRowRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t CollectionSize(
        std::uint32_t collection) = 0;
    [[nodiscard]] virtual std::int32_t RowSize(std::uint32_t row) = 0;
    virtual void LogBind(std::string_view marker,
                         std::uint32_t table_name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void ProcessPropertyRegistry(std::uint32_t binding) = 0;
    virtual void BindEffects(std::uint32_t binding) = 0;
    virtual void InitializeRecord(StoryDataRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupTemporaryRecord(
        StoryDataRecord& record) = 0;
    [[nodiscard]] virtual bool ParseBoundRow(
        std::uint32_t binding, std::uint32_t row,
        StoryDataRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t FindKey(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t key) = 0;
    virtual void InsertRecord(std::uint32_t array,
                              std::uint32_t end,
                              const StoryDataRecord& record) = 0;
    virtual void AppendRecord(std::uint32_t array,
                              std::uint32_t end,
                              const StoryDataRecord& record) = 0;
    virtual void InsertKey(std::uint32_t array,
                           std::uint32_t position,
                           std::uint32_t key) = 0;
    virtual void AssignRecord(std::uint32_t destination,
                              const StoryDataRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t FieldAt(
        std::uint32_t fields, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t PropertyAt(
        std::uint32_t row, std::uint32_t index) = 0;
    virtual void ConvertField(std::uint32_t field,
                              StoryDataRecord& record,
                              std::uint32_t property) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class IStoryDataRefreshRuntime {
public:
    virtual ~IStoryDataRefreshRuntime() = default;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Refresh(std::uint32_t table) = 0;
};

class IStoryDataIndexRuntime {
public:
    virtual ~IStoryDataIndexRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t CopyUnique(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyRecord(
        std::uint32_t record) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t end,
                           std::uint32_t key, std::uint32_t index) = 0;
};

class IStoryDataCapacityRuntime {
public:
    virtual ~IStoryDataCapacityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void DestroyRecord(std::uint32_t record) = 0;
    virtual void CopyIndex(std::uint32_t destination,
                           std::uint32_t source) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class StoryDataTable final {
public:
    // IDA 0x4faff0: construct the 140-byte Effect-derived Story table.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t name, IStoryDataTableRuntime& runtime);
    // IDA 0x4fb0c0: release rows, properties and the Effect base.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t table, IStoryDataTableDestroyRuntime& runtime);
    // IDA 0x4fb200: parse and insert or replace a Story row.
    [[nodiscard]] static std::uint8_t AddOrReplace(
        std::uint32_t table, std::uint32_t row,
        IStoryDataRowRuntime& runtime);
    // IDA 0x4fb480: convert and append a Story row.
    [[nodiscard]] static std::uint32_t Append(
        std::uint32_t table, std::uint32_t row,
        IStoryDataRowRuntime& runtime);
    // IDA 0x4fb620: mark the table dirty and refresh.
    [[nodiscard]] static std::uint32_t MarkAndRefresh(
        std::uint32_t table, IStoryDataRefreshRuntime& runtime);
    // IDA 0x4fb630: compact duplicate Story rows and rebuild signed-ID index.
    static void RebuildIndex(std::uint32_t table,
                             IStoryDataIndexRuntime& runtime);
    // IDA 0x4fb870: destroy a half-open range of 36-byte Story rows.
    [[nodiscard]] static std::uint32_t DestroyRange(
        std::uint32_t begin, std::uint32_t end,
        IStoryDataIndexRuntime& runtime);
    // IDA 0x4fb8a0: independently reserve row and key-index capacity.
    [[nodiscard]] static std::uint32_t Reserve(
        std::uint32_t table, std::int32_t requested,
        IStoryDataCapacityRuntime& runtime);
    // IDA 0x4fdf70: refresh Story field bindings on a count mismatch.
    [[nodiscard]] static std::uint32_t RefreshBindings(
        std::uint32_t table, IGameDataTableBindingRefreshRuntime& runtime);
};

}  // namespace pal4
