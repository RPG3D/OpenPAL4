#include "pal4/story_desc_record_array.h"
#include "record_array_common.h"

namespace pal4 {

std::uint32_t StoryDescRecordArray::Insert(
    std::uint32_t array, std::uint32_t position,
    std::uint32_t source, IStoryRecordArrayRuntime& runtime) {
    return detail::InsertRecord<36>(array, position, source, runtime);
}

std::uint32_t StoryDescRecordArray::CopyRange(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination, IStoryRecordArrayRuntime& runtime) {
    return detail::CopyRecordRange<36>(begin, end, destination, runtime);
}

void StoryDescRecordArray::InsertCopies(
    std::uint32_t array, std::uint32_t position,
    std::uint32_t count, std::uint32_t source,
    IStoryRecordArrayRuntime& runtime) {
    detail::InsertRecordCopies<36>(array, position, count, source, runtime);
}

void StoryDescRecordArray::Fill(
    std::uint32_t destination, std::uint32_t count,
    std::uint32_t source, IStoryRecordArrayRuntime& runtime) {
    for (std::uint32_t index = 0; index < count; ++index)
        runtime.CopyRecord(destination + index * 36, source);
}

}  // namespace pal4
