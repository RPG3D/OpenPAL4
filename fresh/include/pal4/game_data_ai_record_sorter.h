#pragma once

#include "pal4/game_data_ai_record_table.h"
#include "pal4/game_data_ai_property_table.h"

#include <cstdint>
#include <span>
#include <vector>

namespace pal4 {

struct GameDataAiKeyEntry {
    std::uint32_t key = 0;
    std::uint32_t record_index = 0;
};

class GameDataAiKeyAlgorithms final {
public:
    // IDA 0x4ce870, 0x4ce970, 0x4cead0: signed-key quick partition
    // followed by insertion of the remaining short ranges.
    [[nodiscard]] static std::size_t PartitionLarge(
        std::vector<GameDataAiKeyEntry>& entries, std::size_t first,
        std::size_t last);
    [[nodiscard]] static std::uint32_t InsertSorted(
        std::vector<GameDataAiKeyEntry>& entries, std::size_t first,
        std::size_t position, GameDataAiKeyEntry entry);
    static void InsertionSort(std::vector<GameDataAiKeyEntry>& entries,
                              std::size_t first, std::size_t last);
    static void Sort(std::vector<GameDataAiKeyEntry>& entries);
    // IDA 0x4ce9b0: lower bound in a sorted key array.
    [[nodiscard]] static std::size_t LowerBound(
        std::span<const GameDataAiKeyEntry> entries, std::uint32_t key);
    // IDA 0x4ceb50: median of three whole key/index entries.
    [[nodiscard]] static GameDataAiKeyEntry SelectMedian(
        GameDataAiKeyEntry first, GameDataAiKeyEntry middle,
        GameDataAiKeyEntry last);
    // IDA 0x4ceb90: partition a half-open range around a signed key.
    [[nodiscard]] static std::size_t Partition(
        std::vector<GameDataAiKeyEntry>& entries, std::size_t first,
        std::size_t last, std::uint32_t pivot);
};

class GameDataAiRecordCompactor final {
public:
    // IDA 0x4ce9f0/0x4cea60: retain the first record in each adjacent run.
    [[nodiscard]] static std::size_t Compact56(
        std::span<const GameDataAiRecord> source,
        std::span<GameDataAiRecord> destination);
    [[nodiscard]] static std::size_t Compact68(
        std::span<const GameDataAiPropertyRecord> source,
        std::span<GameDataAiPropertyRecord> destination);
};

class IGameDataAiRecordSortRuntime {
public:
    virtual ~IGameDataAiRecordSortRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual GameDataAiRecord ReadRecord(
        std::uint32_t address) = 0;
    virtual void WriteRecord(std::uint32_t address,
                             const GameDataAiRecord& record) = 0;
    virtual void AppendKey(std::uint32_t array,
                           const GameDataAiKeyEntry& entry) = 0;
    virtual void WriteKey(std::uint32_t address,
                          const GameDataAiKeyEntry& entry) = 0;
};

class GameDataAiRecordSorter final {
public:
    // IDA 0x4cc7b0: compact adjacent duplicate records and rebuild keys.
    static void Sort(std::uint32_t table,
                     IGameDataAiRecordSortRuntime& runtime);
};

class IGameDataAiPropertySortRuntime {
public:
    virtual ~IGameDataAiPropertySortRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual GameDataAiPropertyRecord ReadRecord(
        std::uint32_t address) = 0;
    virtual void WriteRecord(std::uint32_t address,
                             const GameDataAiPropertyRecord& record) = 0;
    virtual void AppendKey(std::uint32_t array,
                           const GameDataAiKeyEntry& entry) = 0;
    virtual void WriteKey(std::uint32_t address,
                          const GameDataAiKeyEntry& entry) = 0;
};

class GameDataAiPropertySorter final {
public:
    // IDA 0x4cd1e0: compact adjacent 68-byte records and rebuild keys.
    static void Sort(std::uint32_t table,
                     IGameDataAiPropertySortRuntime& runtime);
};

}  // namespace pal4
