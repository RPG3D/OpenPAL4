#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

#include "pal4/game_data_ai_record_sorter.h"

namespace pal4 {

using GameDataUpgradeParameterRecord = std::array<std::uint32_t, 23>;

class IGameDataUpgradeParameterTableRuntime {
public:
    virtual ~IGameDataUpgradeParameterTableRuntime() = default;
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
                              GameDataUpgradeParameterRecord& record,
                              std::uint32_t property) = 0;
    [[nodiscard]] virtual std::uint32_t FindKey(std::uint32_t begin,
                                                std::uint32_t end,
                                                std::uint32_t key) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            const GameDataUpgradeParameterRecord& record) = 0;
    virtual void InsertRecord(std::uint32_t array, std::uint32_t position,
                              const GameDataUpgradeParameterRecord& record) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t position,
                           std::uint32_t key, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t AppendRecord(
        std::uint32_t array, std::uint32_t end,
        const GameDataUpgradeParameterRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(std::uint32_t table,
                                              std::uint32_t row) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Refresh(std::uint32_t table) = 0;
};

class GameDataUpgradeParameterTable final {
public:
    // IDA 0x4d00f0: bind fields if needed, then insert or replace by key.
    [[nodiscard]] static std::uint8_t AddOrReplace(
        std::uint32_t table, std::uint32_t row,
        IGameDataUpgradeParameterTableRuntime& runtime);
    // IDA 0x4d03f0: convert and append a 92-byte record.
    [[nodiscard]] static std::uint32_t Append(
        std::uint32_t table, std::uint32_t row,
        IGameDataUpgradeParameterTableRuntime& runtime);
    // IDA 0x4d0550: mark the table dirty, then refresh it.
    [[nodiscard]] static std::uint32_t MarkAndRefresh(
        std::uint32_t table, IGameDataUpgradeParameterTableRuntime& runtime);
};

class IGameDataUpgradeParameterStorageRuntime {
public:
    virtual ~IGameDataUpgradeParameterStorageRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual GameDataUpgradeParameterRecord ReadRecord(
        std::uint32_t address) = 0;
    virtual void WriteRecord(std::uint32_t address,
                             const GameDataUpgradeParameterRecord& record) = 0;
    [[nodiscard]] virtual GameDataAiKeyEntry ReadKey(
        std::uint32_t address) = 0;
    virtual void WriteKey(std::uint32_t address,
                          const GameDataAiKeyEntry& entry) = 0;
    virtual void AppendKey(std::uint32_t array,
                           const GameDataAiKeyEntry& entry) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class GameDataUpgradeParameterStorage final {
public:
    // IDA 0x4d0560: compact adjacent equal tail keys and rebuild the key map.
    static void Sort(std::uint32_t table,
                     IGameDataUpgradeParameterStorageRuntime& runtime);
    // IDA 0x4d07a0: independently grow the record and key arrays.
    [[nodiscard]] static std::uint32_t Reserve(
        std::uint32_t table, std::uint32_t requested,
        IGameDataUpgradeParameterStorageRuntime& runtime);
    // IDA 0x4d1250: compare the tail keys of two 92-byte records.
    [[nodiscard]] static bool EqualTailKeys(
        const GameDataUpgradeParameterRecord& left,
        const GameDataUpgradeParameterRecord& right);
};

}  // namespace pal4
