#pragma once

#include <cstdint>
#include <functional>

namespace pal4 {

class IGameDataMonsterRecordCopyRuntime {
public:
    virtual ~IGameDataMonsterRecordCopyRuntime() = default;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void AssignText(std::uint32_t destination,
                            std::uint32_t source) = 0;
};

class GameDataMonsterRecordCopy final {
public:
    // IDA 0x4e00c0: assign one record to each destination in a range.
    [[nodiscard]] static std::uint32_t AssignRange(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t source,
        IGameDataMonsterRecordCopyRuntime& runtime);
    // IDA 0x4e0340: assign an overlapping range backward.
    [[nodiscard]] static std::uint32_t AssignBackward(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination_end,
        IGameDataMonsterRecordCopyRuntime& runtime);
    // IDA 0x4e05c0: deep copy-construct a 596-byte Monster record.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t destination, std::uint32_t source,
        IGameDataMonsterRecordCopyRuntime& runtime);
    static void Assign(std::uint32_t destination,
                       std::uint32_t source,
                       IGameDataMonsterRecordCopyRuntime& runtime);
    // IDA 0x4e0770: assign a complete record and return its destination.
    [[nodiscard]] static std::uint32_t AssignRecord(
        std::uint32_t destination, std::uint32_t source,
        IGameDataMonsterRecordCopyRuntime& runtime);
    // IDA 0x4e0a30: assign the 260-byte subrecord at record +336.
    [[nodiscard]] static std::uint32_t AssignSubrecord(
        std::uint32_t destination, std::uint32_t source,
        IGameDataMonsterRecordCopyRuntime& runtime);
    // IDA 0x4e0d70: copy the first record, then copy records not matched
    // by the predicate against the last retained source record.
    [[nodiscard]] static std::uint32_t CopySelected(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination,
        const std::function<bool(std::uint32_t, std::uint32_t)>& equivalent,
        IGameDataMonsterRecordCopyRuntime& runtime);
};

}  // namespace pal4
