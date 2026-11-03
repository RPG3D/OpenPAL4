#pragma once

#include <cstdint>

namespace pal4 {

class IStoryRecordArrayRuntime {
public:
    virtual ~IStoryRecordArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void AssignRecord(std::uint32_t destination,
                              std::uint32_t source) = 0;
    virtual void DestroyRecord(std::uint32_t record) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class StoryRecordArray final {
public:
    // IDA 0x4fce10: insert one 36-byte record at a physical array position.
    [[nodiscard]] static std::uint32_t Insert(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t source, IStoryRecordArrayRuntime& runtime);
    // IDA 0x4fcff0: copy-construct a half-open range of 36-byte records.
    [[nodiscard]] static std::uint32_t CopyRange(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination, IStoryRecordArrayRuntime& runtime);
    // IDA 0x5067e0: second 36-byte Story insertion instantiation.
    [[nodiscard]] static std::uint32_t InsertCallback(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t source, IStoryRecordArrayRuntime& runtime);
    // IDA 0x5069c0: second 36-byte Story range-copy instantiation.
    [[nodiscard]] static std::uint32_t CopyCallbackRange(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination, IStoryRecordArrayRuntime& runtime);
    // IDA 0x5078a0: second repeated 36-byte Story insertion instantiation.
    static void InsertCallbackCopies(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t count, std::uint32_t source,
        IStoryRecordArrayRuntime& runtime);
    // IDA 0x508650: second repeated 36-byte Story copy instantiation.
    static void FillCallback(std::uint32_t destination,
                             std::uint32_t count,
                             std::uint32_t source,
                             IStoryRecordArrayRuntime& runtime);
    // IDA 0x4fd210: count active 36-byte records, or zero for null storage.
    [[nodiscard]] static std::uint32_t Size(
        std::uint32_t array, IStoryRecordArrayRuntime& runtime);
    // IDA 0x4fd4e0: insert repeated copies of a 36-byte Story record.
    static void InsertCopies(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t count, std::uint32_t source,
        IStoryRecordArrayRuntime& runtime);
    // IDA 0x4fdee0: copy-construct count Story records from one source.
    static void Fill(std::uint32_t destination, std::uint32_t count,
                     std::uint32_t source, IStoryRecordArrayRuntime& runtime);
};

}  // namespace pal4
