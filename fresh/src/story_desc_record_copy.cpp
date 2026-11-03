#include "pal4/story_desc_record_copy.h"

namespace pal4 {

void StoryDescRecordCopy::Construct(
    std::uint32_t destination, std::uint32_t source,
    IStoryRecordCopyRuntime& runtime) {
    StoryRecordCopy::Construct(destination, source, runtime);
}

}  // namespace pal4
