#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

#include "pal4/game_data_ai_record_sorter.h"

namespace pal4 {

class IGameDataDropPropertyTableRuntime {
public:
    virtual ~IGameDataDropPropertyTableRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t CollectionSize(std::uint32_t collection) = 0;
    [[nodiscard]] virtual std::uint32_t RowSize(std::uint32_t row) = 0;
    virtual void LogBind(std::string_view prefix, std::uint32_t table_name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void InitializeStructures(std::uint32_t object) = 0;
    virtual void BindEffects(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t PropertyAt(std::uint32_t row,
                                                   std::uint32_t index) = 0;
    virtual void ConvertField(std::uint32_t field, std::uint32_t& value,
                              std::uint32_t property) = 0;
    [[nodiscard]] virtual std::uint32_t FindKey(std::uint32_t begin,
                                                std::uint32_t end,
                                                std::uint32_t key) = 0;
    virtual void InsertValue(std::uint32_t array, std::uint32_t position,
                             std::uint32_t value) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t position,
                           std::uint32_t key, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t AppendValue(
        std::uint32_t array, std::uint32_t end,
        std::uint32_t value) = 0;
    [[nodiscard]] virtual std::string DumpRow(std::uint32_t table,
                                              std::uint32_t row) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Refresh(std::uint32_t table) = 0;
};

class GameDataDropPropertyTable final {
public:
    // IDA 0x4d0b00: bind fields, then insert or replace a scalar value.
    [[nodiscard]] static std::uint8_t AddOrReplace(
        std::uint32_t table, std::uint32_t row,
        IGameDataDropPropertyTableRuntime& runtime);
    // IDA 0x4d0dc0: convert a row and append its scalar value.
    [[nodiscard]] static std::uint32_t Append(
        std::uint32_t table, std::uint32_t row,
        IGameDataDropPropertyTableRuntime& runtime);
    // IDA 0x4d0f10: mark the table dirty and refresh.
    [[nodiscard]] static std::uint32_t MarkAndRefresh(
        std::uint32_t table, IGameDataDropPropertyTableRuntime& runtime);
};

class IGameDataDropPropertyStorageRuntime {
public:
    virtual ~IGameDataDropPropertyStorageRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual GameDataAiKeyEntry ReadKey(
        std::uint32_t address) = 0;
    virtual void WriteKey(std::uint32_t address,
                          const GameDataAiKeyEntry& entry) = 0;
    virtual void AppendKey(std::uint32_t array,
                           const GameDataAiKeyEntry& entry) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class GameDataDropPropertyStorage final {
public:
    using ValueEqual = std::function<bool(std::uint32_t, std::uint32_t)>;
    // IDA 0x4d0f20: compact adjacent duplicate values and sort their keys.
    static void Sort(std::uint32_t table,
                     IGameDataDropPropertyStorageRuntime& runtime);
    // IDA 0x4d1130: grow scalar and key arrays independently.
    [[nodiscard]] static std::uint32_t Reserve(
        std::uint32_t table, std::uint32_t requested,
        IGameDataDropPropertyStorageRuntime& runtime);
    // IDA 0x4d1d80: compact successive equal DWORD values.
    [[nodiscard]] static std::uint32_t CompactAdjacent(
        std::uint32_t begin, std::uint32_t end, std::uint32_t destination,
        const ValueEqual& equal,
        IGameDataDropPropertyStorageRuntime& runtime);
};

}  // namespace pal4
