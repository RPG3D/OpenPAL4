#pragma once

#include <cstdint>
#include <functional>

#include "pal4/game_data_upgrade_parameter_table.h"

namespace pal4 {

class IGameDataUpgradeParameterValueArrayRuntime {
public:
    virtual ~IGameDataUpgradeParameterValueArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual GameDataUpgradeParameterRecord ReadRecord(
        std::uint32_t address) = 0;
    virtual void WriteRecord(
        std::uint32_t address,
        const GameDataUpgradeParameterRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class GameDataUpgradeParameterValueArray final {
public:
    using RecordEqual = std::function<bool(
        const GameDataUpgradeParameterRecord&,
        const GameDataUpgradeParameterRecord&)>;
    // IDA 0x4d1270: insert one 92-byte record at a specified position.
    [[nodiscard]] static std::uint32_t Insert(
        std::uint32_t array, std::uint32_t position,
        const GameDataUpgradeParameterRecord& record,
        IGameDataUpgradeParameterValueArrayRuntime& runtime);
    // IDA 0x4d1490: return the live record count.
    [[nodiscard]] static std::uint32_t Size(
        std::uint32_t array,
        IGameDataUpgradeParameterValueArrayRuntime& runtime);
    // IDA 0x4d14c0: copy a half-open range in 92-byte steps.
    [[nodiscard]] static std::uint32_t CopyRecords(
        std::uint32_t begin, std::uint32_t end, std::uint32_t destination,
        IGameDataUpgradeParameterValueArrayRuntime& runtime);
    // IDA 0x4d1500: insert repeated copies of one record.
    [[nodiscard]] static std::uint32_t InsertCopies(
        std::uint32_t array, std::uint32_t position, std::uint32_t count,
        const GameDataUpgradeParameterRecord& record,
        IGameDataUpgradeParameterValueArrayRuntime& runtime);
    // IDA 0x4d1830: fill a range with copies of one record.
    [[nodiscard]] static std::uint32_t FillRecords(
        std::uint32_t destination, std::uint32_t count,
        const GameDataUpgradeParameterRecord& record,
        IGameDataUpgradeParameterValueArrayRuntime& runtime);
    // IDA 0x4d1cc0: a null destination suppresses the 92-byte copy.
    static void CopyRecordIfPresent(
        std::uint32_t destination, std::uint32_t source,
        IGameDataUpgradeParameterValueArrayRuntime& runtime);
    // IDA 0x4d1d10: compact successive equal records into destination.
    [[nodiscard]] static std::uint32_t CompactAdjacent(
        std::uint32_t begin, std::uint32_t end, std::uint32_t destination,
        const RecordEqual& equal,
        IGameDataUpgradeParameterValueArrayRuntime& runtime);
};

}  // namespace pal4
