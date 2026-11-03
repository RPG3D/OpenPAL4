#pragma once

#include "pal4/story_data_table.h"

#include <cstdint>
#include <array>
#include <string>
#include <string_view>

namespace pal4 {

class IGameQuestionDataTableDestroyRuntime {
public:
    virtual ~IGameQuestionDataTableDestroyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void ClearEffectData(std::uint32_t table) = 0;
    virtual void DestroyQuestionRecord(std::uint32_t record) = 0;
    virtual void ProcessElements(std::uint32_t elements) = 0;
    virtual void DestroyPropertyRecord(std::uint32_t record) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyBase(
        std::uint32_t table) = 0;
};

class IGameQuestionDataStorageRuntime {
public:
    virtual ~IGameQuestionDataStorageRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ClearEffectData(
        std::uint32_t table) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyQuestionRecord(
        std::uint32_t record) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

using GameQuestionDataRecord = std::array<std::uint32_t, 19>;

class IGameQuestionDataRowRuntime {
public:
    virtual ~IGameQuestionDataRowRuntime() = default;
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
    virtual void InitializeRecord(GameQuestionDataRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupTemporaryRecord(
        GameQuestionDataRecord& record) = 0;
    [[nodiscard]] virtual bool ParseBoundRow(
        std::uint32_t binding, std::uint32_t row,
        GameQuestionDataRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t FindKey(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t key) = 0;
    virtual void InsertRecord(std::uint32_t array,
                              std::uint32_t end,
                              const GameQuestionDataRecord& record) = 0;
    virtual void AppendRecord(std::uint32_t array,
                              std::uint32_t end,
                              const GameQuestionDataRecord& record) = 0;
    virtual void InsertKey(std::uint32_t array,
                           std::uint32_t position,
                           std::uint32_t key) = 0;
    virtual void AssignRecord(std::uint32_t destination,
                              const GameQuestionDataRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t FieldAt(
        std::uint32_t fields, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t PropertyAt(
        std::uint32_t row, std::uint32_t index) = 0;
    virtual void ConvertField(std::uint32_t field,
                              GameQuestionDataRecord& record,
                              std::uint32_t property) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class IGameQuestionDataIndexRuntime {
public:
    virtual ~IGameQuestionDataIndexRuntime() = default;
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

class IGameQuestionDataCapacityRuntime {
public:
    virtual ~IGameQuestionDataCapacityRuntime() = default;
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

class GameQuestionDataTable final {
public:
    // IDA 0x4fc1c0: construct a 140-byte GameQuestion table.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t name, IStoryDataTableRuntime& runtime);
    // IDA 0x4fc290: release 76-byte question rows and table storage.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t table,
        IGameQuestionDataTableDestroyRuntime& runtime);
    // IDA 0x4fc3c0: destroy and free a standalone question record array.
    [[nodiscard]] static std::uint32_t ReleaseRecordArray(
        std::uint32_t array, IGameQuestionDataStorageRuntime& runtime);
    // IDA 0x4fc400: clear active rows without freeing their capacity.
    [[nodiscard]] static std::uint32_t ClearRows(
        std::uint32_t table, IGameQuestionDataStorageRuntime& runtime);
    // IDA 0x4fc490: parse and insert or replace a 76-byte question row.
    [[nodiscard]] static std::uint8_t AddOrReplace(
        std::uint32_t table, std::uint32_t row,
        IGameQuestionDataRowRuntime& runtime);
    // IDA 0x4fc760: convert and append a question row.
    [[nodiscard]] static std::uint32_t Append(
        std::uint32_t table, std::uint32_t row,
        IGameQuestionDataRowRuntime& runtime);
    // IDA 0x4fc940: mark dirty and refresh question fields.
    [[nodiscard]] static std::uint32_t MarkAndRefresh(
        std::uint32_t table, IStoryDataRefreshRuntime& runtime);
    // IDA 0x4fc950: compact duplicate 76-byte rows and rebuild the ID index.
    static void RebuildIndex(std::uint32_t table,
                             IGameQuestionDataIndexRuntime& runtime);
    // IDA 0x4fcb90: destroy a half-open range of question rows.
    [[nodiscard]] static std::uint32_t DestroyRange(
        std::uint32_t begin, std::uint32_t end,
        IGameQuestionDataIndexRuntime& runtime);
    // IDA 0x4fcbc0: independently reserve question rows and index entries.
    [[nodiscard]] static std::uint32_t Reserve(
        std::uint32_t table, std::int32_t requested,
        IGameQuestionDataCapacityRuntime& runtime);
    // IDA 0x4fe3d0: refresh GameQuestion field bindings on mismatch.
    [[nodiscard]] static std::uint32_t RefreshBindings(
        std::uint32_t table, IGameDataTableBindingRefreshRuntime& runtime);
};

}  // namespace pal4
