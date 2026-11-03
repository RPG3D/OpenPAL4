#pragma once

#include "pal4/utilities_record_text_field.h"

#include <cstdint>

namespace pal4 {

class IStoryRecordCopyRuntime {
public:
    virtual ~IStoryRecordCopyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void AssignText(std::uint32_t destination,
                            std::uint32_t source) = 0;
    [[nodiscard]] virtual bool RecordsEqual(
        std::uint32_t left, std::uint32_t right) = 0;
};

class StoryRecordCopy final {
public:
    // IDA 0x4ff260: assign one existing Story record.
    [[nodiscard]] static std::uint32_t Assign(
        std::uint32_t destination, std::uint32_t source,
        IStoryRecordCopyRuntime& runtime);
    // IDA 0x4fe600: copy-construct a 36-byte Story record.
    static void Construct(std::uint32_t destination, std::uint32_t source,
                          IStoryRecordCopyRuntime& runtime);
    // IDA 0x509fb0: copy-construct the callback Story record variant.
    static void ConstructCallback(
        std::uint32_t destination, std::uint32_t source,
        IUtilitiesRecordTextRuntime& runtime);
    // IDA 0x4fe750: assign one source throughout a half-open Story range.
    [[nodiscard]] static std::uint32_t AssignRepeated(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t source, IStoryRecordCopyRuntime& runtime);
    // IDA 0x4fe950: move a Story range backward into destination_end.
    [[nodiscard]] static std::uint32_t AssignBackward(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination_end, IStoryRecordCopyRuntime& runtime);
    // IDA 0x4ff6f0: retain the first row of each adjacent equal run.
    [[nodiscard]] static std::uint32_t CopyUnique(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination, IStoryRecordCopyRuntime& runtime);
};

}  // namespace pal4
