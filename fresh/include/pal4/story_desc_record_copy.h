#pragma once

#include "pal4/story_record_copy.h"

namespace pal4 {

class StoryDescRecordCopy final {
public:
    // IDA 0x4feb70: copy-construct a 36-byte StoryDesc record.
    static void Construct(std::uint32_t destination, std::uint32_t source,
                          IStoryRecordCopyRuntime& runtime);
};

}  // namespace pal4
