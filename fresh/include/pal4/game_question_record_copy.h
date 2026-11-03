#pragma once

#include <cstdint>

namespace pal4 {

class IGameQuestionRecordCopyRuntime {
public:
    virtual ~IGameQuestionRecordCopyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void SetErrorFlag(std::uint32_t text, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t AssignText(
        std::uint32_t destination, std::uint32_t source) = 0;
    [[nodiscard]] virtual bool RecordsEqual(
        std::uint32_t left, std::uint32_t right) = 0;
};

class GameQuestionRecordCopy final {
public:
    // IDA 0x4ff490: assign one existing question record.
    [[nodiscard]] static std::uint32_t Assign(
        std::uint32_t destination, std::uint32_t source,
        IGameQuestionRecordCopyRuntime& runtime);
    // IDA 0x4ff150: copy-construct one 76-byte question record.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t destination, std::uint32_t source,
        IGameQuestionRecordCopyRuntime& runtime);
    // IDA 0x4fecc0: assign one question row to every record in a range.
    [[nodiscard]] static std::uint32_t AssignRepeated(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t source, IGameQuestionRecordCopyRuntime& runtime);
    // IDA 0x4feef0: assign a question range backward into destination_end.
    [[nodiscard]] static std::uint32_t AssignBackward(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination_end,
        IGameQuestionRecordCopyRuntime& runtime);
    // IDA 0x4ff910: compact adjacent equal question rows.
    [[nodiscard]] static std::uint32_t CopyUnique(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination,
        IGameQuestionRecordCopyRuntime& runtime);
};

}  // namespace pal4
