#pragma once

#include "pal4/game_data_ai_property_table.h"
#include "pal4/game_data_ai_record_table.h"

#include <cstdint>

namespace pal4 {

class IGameDataAiValueArrayRuntime {
public:
    virtual ~IGameDataAiValueArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual GameDataAiRecord ReadRecord(
        std::uint32_t address) = 0;
    virtual void WriteRecord(std::uint32_t address,
                             const GameDataAiRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class GameDataAiValueArray final {
public:
    // IDA 0x4cd800: insert a 56-byte record at a specified position.
    [[nodiscard]] static std::uint32_t Insert(
        std::uint32_t array, std::uint32_t position,
        const GameDataAiRecord& record, IGameDataAiValueArrayRuntime& runtime);
    // IDA 0x4cda20: copy [begin, end) in 56-byte steps.
    [[nodiscard]] static std::uint32_t CopyRecords(
        std::uint32_t begin, std::uint32_t end, std::uint32_t destination,
        IGameDataAiValueArrayRuntime& runtime);
    // IDA 0x4cdce0: insert multiple copies of one 56-byte record.
    [[nodiscard]] static std::uint32_t InsertCopies(
        std::uint32_t array, std::uint32_t position, std::uint32_t count,
        const GameDataAiRecord& record, IGameDataAiValueArrayRuntime& runtime);
    // IDA 0x4ce310: copy one 56-byte record a requested number of times.
    [[nodiscard]] static std::uint32_t FillRecords(
        std::uint32_t destination, std::uint32_t count,
        const GameDataAiRecord& record, IGameDataAiValueArrayRuntime& runtime);
    // IDA 0x4ce800: copy one 56-byte record when destination is non-null.
    static void CopyOne(std::uint32_t destination, std::uint32_t source,
                        IGameDataAiValueArrayRuntime& runtime);
};

class IGameDataAiPropertyValueArrayRuntime {
public:
    virtual ~IGameDataAiPropertyValueArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual GameDataAiPropertyRecord ReadRecord(
        std::uint32_t address) = 0;
    virtual void WriteRecord(std::uint32_t address,
                             const GameDataAiPropertyRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class GameDataAiPropertyValueArray final {
public:
    // IDA 0x4cdc70: return used 68-byte property-record count.
    [[nodiscard]] static std::uint32_t Size(
        std::uint32_t array, IGameDataAiPropertyValueArrayRuntime& runtime);
    // IDA 0x4cdca0: copy [begin, end) in 68-byte steps.
    [[nodiscard]] static std::uint32_t CopyRecords(
        std::uint32_t begin, std::uint32_t end, std::uint32_t destination,
        IGameDataAiPropertyValueArrayRuntime& runtime);
    // IDA 0x4cda60: insert a 68-byte record at a specified position.
    [[nodiscard]] static std::uint32_t Insert(
        std::uint32_t array, std::uint32_t position,
        const GameDataAiPropertyRecord& record,
        IGameDataAiPropertyValueArrayRuntime& runtime);
    // IDA 0x4ce000: insert multiple copies of one 68-byte record.
    [[nodiscard]] static std::uint32_t InsertCopies(
        std::uint32_t array, std::uint32_t position, std::uint32_t count,
        const GameDataAiPropertyRecord& record,
        IGameDataAiPropertyValueArrayRuntime& runtime);
    // IDA 0x4ce340: copy one 68-byte record a requested number of times.
    [[nodiscard]] static std::uint32_t FillRecords(
        std::uint32_t destination, std::uint32_t count,
        const GameDataAiPropertyRecord& record,
        IGameDataAiPropertyValueArrayRuntime& runtime);
    // IDA 0x4ce820: copy one 68-byte record when destination is non-null.
    static void CopyOne(std::uint32_t destination, std::uint32_t source,
                        IGameDataAiPropertyValueArrayRuntime& runtime);
};

}  // namespace pal4
