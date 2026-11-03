#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

#include "pal4/game_data_ai_record_sorter.h"

namespace pal4 {

using GameDataEquipRelationRecord = std::array<std::uint32_t, 3>;

class IGameDataEquipRelationTableRuntime {
public:
    virtual ~IGameDataEquipRelationTableRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CollectionSize(std::uint32_t collection) = 0;
    [[nodiscard]] virtual std::uint32_t RowSize(std::uint32_t row) = 0;
    virtual void LogBind(std::string_view prefix, std::uint32_t table_name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void InitializeStructures(std::uint32_t object) = 0;
    virtual void BindEffects(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t PropertyAt(std::uint32_t row,
                                                   std::uint32_t index) = 0;
    virtual void ConvertField(std::uint32_t field,
                              GameDataEquipRelationRecord& record,
                              std::uint32_t property) = 0;
    [[nodiscard]] virtual bool CheckMembersRange(
        const GameDataEquipRelationRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t FindKey(std::uint32_t begin,
                                                std::uint32_t end,
                                                std::uint32_t key) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            const GameDataEquipRelationRecord& record) = 0;
    virtual void InsertRecord(std::uint32_t array, std::uint32_t position,
                              const GameDataEquipRelationRecord& record) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t position,
                           std::uint32_t key, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t AppendRecord(
        std::uint32_t array, std::uint32_t end,
        const GameDataEquipRelationRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(std::uint32_t table,
                                              std::uint32_t row) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Refresh(std::uint32_t table) = 0;
};

class GameDataEquipRelationTable final {
public:
    // IDA 0x4d5850: bind fields if needed, then insert or replace by ID.
    [[nodiscard]] static std::uint8_t AddOrReplace(
        std::uint32_t table, std::uint32_t row,
        IGameDataEquipRelationTableRuntime& runtime);
    // IDA 0x4d5b40: convert and append a 12-byte relation record.
    [[nodiscard]] static std::uint32_t Append(
        std::uint32_t table, std::uint32_t row,
        IGameDataEquipRelationTableRuntime& runtime);
    // IDA 0x4d5c80: mark dirty and refresh the table.
    [[nodiscard]] static std::uint32_t MarkAndRefresh(
        std::uint32_t table, IGameDataEquipRelationTableRuntime& runtime);
};

class IGameDataEquipRelationStorageRuntime {
public:
    virtual ~IGameDataEquipRelationStorageRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual GameDataEquipRelationRecord ReadRecord(
        std::uint32_t address) = 0;
    virtual void WriteRecord(std::uint32_t address,
                             const GameDataEquipRelationRecord& record) = 0;
    virtual void AppendKey(std::uint32_t array,
                           const GameDataAiKeyEntry& entry) = 0;
    virtual void WriteKey(std::uint32_t address,
                          const GameDataAiKeyEntry& entry) = 0;
    [[nodiscard]] virtual GameDataAiKeyEntry ReadKey(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class GameDataEquipRelationStorage final {
public:
    // IDA 0x4d5c90: compact adjacent IDs and rebuild the signed key map.
    static void Sort(std::uint32_t table,
                     IGameDataEquipRelationStorageRuntime& runtime);
    // IDA 0x4d5ee0: independently grow the relation and key arrays.
    [[nodiscard]] static std::uint32_t Reserve(
        std::uint32_t table, std::uint32_t requested,
        IGameDataEquipRelationStorageRuntime& runtime);
    // IDA 0x4d6cd0: insert one 12-byte record into a three-pointer array.
    [[nodiscard]] static std::uint32_t Insert(
        std::uint32_t array, std::uint32_t position,
        const GameDataEquipRelationRecord& record,
        IGameDataEquipRelationStorageRuntime& runtime);
    // IDA 0x4d79b0: copy a 12-byte record only for a non-null destination.
    [[nodiscard]] static std::uint32_t CopyIfAllocated(
        std::uint32_t destination, std::uint32_t source,
        IGameDataEquipRelationStorageRuntime& runtime);
    // IDA 0x4d8210: retain the first record of each adjacent equal run.
    [[nodiscard]] static std::uint32_t CompactAdjacent(
        std::uint32_t first, std::uint32_t last,
        std::uint32_t destination,
        bool (*equal)(const GameDataEquipRelationRecord&,
                      const GameDataEquipRelationRecord&),
        IGameDataEquipRelationStorageRuntime& runtime);
};

}  // namespace pal4
