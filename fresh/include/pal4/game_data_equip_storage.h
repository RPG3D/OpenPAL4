#pragma once

#include <cstdint>

#include "pal4/game_data_ai_record_sorter.h"
#include "pal4/game_data_equip_record.h"

namespace pal4 {

class IGameDataEquipStorageRuntime : public IGameDataEquipRecordRuntime {
public:
    virtual ~IGameDataEquipStorageRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual GameDataEquipRecord ReadRecord(
        std::uint32_t address) = 0;
    // Assignment releases destination text and retains or copies source text.
    virtual void AssignRecord(std::uint32_t destination,
                              std::uint32_t source) = 0;
    // Construction initializes a fresh record from an existing one.
    virtual void CopyConstructRecord(std::uint32_t destination,
                                     std::uint32_t source) = 0;
    virtual void AppendKey(std::uint32_t array,
                           const GameDataAiKeyEntry& entry) = 0;
    virtual void WriteKey(std::uint32_t address,
                          const GameDataAiKeyEntry& entry) = 0;
    [[nodiscard]] virtual GameDataAiKeyEntry ReadKey(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class GameDataEquipStorage final {
public:
    // IDA 0x4d68a0: compact adjacent IDs, clean the tail and sort key pairs.
    static void Sort(std::uint32_t table,
                     IGameDataEquipStorageRuntime& runtime);
    // IDA 0x4d6b20: grow 400-byte record and 8-byte key arrays independently.
    [[nodiscard]] static std::uint32_t Reserve(
        std::uint32_t table, std::uint32_t requested,
        IGameDataEquipStorageRuntime& runtime);
    // IDA 0x4d6ee0: insert a string-owning 400-byte record.
    [[nodiscard]] static std::uint32_t Insert(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t source, IGameDataEquipStorageRuntime& runtime);
    // IDA 0x4d70f0: count live 400-byte records, or zero for null storage.
    [[nodiscard]] static std::uint32_t Size(
        std::uint32_t array, IGameDataEquipStorageRuntime& runtime);
    // IDA 0x4d7120: copy-construct a half-open range into fresh storage.
    [[nodiscard]] static std::uint32_t CopyConstructRange(
        std::uint32_t first, std::uint32_t last, std::uint32_t destination,
        IGameDataEquipStorageRuntime& runtime);
    // IDA 0x4d7160: insert multiple copies of one Equip record.
    [[nodiscard]] static std::uint32_t InsertRepeated(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t count, std::uint32_t source,
        IGameDataEquipStorageRuntime& runtime);
    // IDA 0x4d7520: copy-construct the same source count times.
    [[nodiscard]] static std::uint32_t CopyRepeated(
        std::uint32_t destination, std::uint32_t count,
        std::uint32_t source, IGameDataEquipStorageRuntime& runtime);
    // IDA 0x4d79d0: assign one source to every record in a range.
    [[nodiscard]] static std::uint32_t AssignRepeated(
        std::uint32_t first, std::uint32_t last,
        std::uint32_t source, IGameDataEquipStorageRuntime& runtime);
    // IDA 0x4d7bf0: copy-construct a range backward into its destination end.
    [[nodiscard]] static std::uint32_t CopyConstructReverse(
        std::uint32_t first, std::uint32_t last,
        std::uint32_t destination_end,
        IGameDataEquipStorageRuntime& runtime);
    // IDA 0x4d8290: compact adjacent equal records with text assignment.
    [[nodiscard]] static std::uint32_t CompactAdjacent(
        std::uint32_t first, std::uint32_t last,
        std::uint32_t destination,
        bool (*equal)(const GameDataEquipRecord&,
                      const GameDataEquipRecord&),
        IGameDataEquipStorageRuntime& runtime);
};

}  // namespace pal4
