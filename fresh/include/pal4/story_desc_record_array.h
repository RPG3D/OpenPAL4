#pragma once

#include "pal4/story_record_array.h"

namespace pal4 {

class StoryDescRecordArray final {
public:
    // IDA 0x4fd030: insert a 36-byte StoryDesc record.
    [[nodiscard]] static std::uint32_t Insert(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t source, IStoryRecordArrayRuntime& runtime);
    // IDA 0x4fd240: copy-construct a half-open StoryDesc record range.
    [[nodiscard]] static std::uint32_t CopyRange(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination, IStoryRecordArrayRuntime& runtime);
    // IDA 0x4fd820: insert repeated copies of a StoryDesc record.
    static void InsertCopies(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t count, std::uint32_t source,
        IStoryRecordArrayRuntime& runtime);
    // IDA 0x4fdf10: copy-construct count StoryDesc records from one source.
    static void Fill(std::uint32_t destination, std::uint32_t count,
                     std::uint32_t source, IStoryRecordArrayRuntime& runtime);
};

}  // namespace pal4
