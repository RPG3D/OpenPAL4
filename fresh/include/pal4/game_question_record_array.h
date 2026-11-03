#pragma once

#include <cstdint>

namespace pal4 {

class IGameQuestionRecordArrayRuntime {
public:
    virtual ~IGameQuestionRecordArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t CopyRecord(
        std::uint32_t destination, std::uint32_t source) = 0;
    virtual void AssignRecord(std::uint32_t destination,
                              std::uint32_t source) = 0;
    virtual void DestroyRecord(std::uint32_t record) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class GameQuestionRecordArray final {
public:
    // IDA 0x4fd280: insert one 76-byte question record.
    [[nodiscard]] static std::uint32_t Insert(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t source, IGameQuestionRecordArrayRuntime& runtime);
    // IDA 0x4fd470: count active 76-byte records.
    [[nodiscard]] static std::uint32_t Size(
        std::uint32_t array, IGameQuestionRecordArrayRuntime& runtime);
    // IDA 0x4fd4a0: copy-construct a half-open range of question records.
    [[nodiscard]] static std::uint32_t CopyRange(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination,
        IGameQuestionRecordArrayRuntime& runtime);
    // IDA 0x4fdb60: insert repeated copies of a 76-byte question record.
    static void InsertCopies(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t count, std::uint32_t source,
        IGameQuestionRecordArrayRuntime& runtime);
    // IDA 0x4fdf40: copy-construct question rows and return the last result.
    [[nodiscard]] static std::uint32_t Fill(
        std::uint32_t destination, std::uint32_t count,
        std::uint32_t source, IGameQuestionRecordArrayRuntime& runtime);
};

}  // namespace pal4
