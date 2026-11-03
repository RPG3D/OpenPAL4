#include "pal4/story_record_array.h"
#include "record_array_common.h"

namespace pal4 {

std::uint32_t StoryRecordArray::Insert(
    std::uint32_t array, std::uint32_t position,
    std::uint32_t source, IStoryRecordArrayRuntime& runtime) {
    return detail::InsertRecord<36>(array, position, source, runtime);
}

std::uint32_t StoryRecordArray::CopyRange(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination, IStoryRecordArrayRuntime& runtime) {
    return detail::CopyRecordRange<36>(begin, end, destination, runtime);
}

std::uint32_t StoryRecordArray::InsertCallback(
    std::uint32_t array, std::uint32_t position,
    std::uint32_t source, IStoryRecordArrayRuntime& runtime) {
    return detail::InsertRecord<36>(array, position, source, runtime);
}

std::uint32_t StoryRecordArray::CopyCallbackRange(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination, IStoryRecordArrayRuntime& runtime) {
    return detail::CopyRecordRange<36>(begin, end, destination, runtime);
}

void StoryRecordArray::InsertCallbackCopies(
    std::uint32_t array, std::uint32_t position,
    std::uint32_t count, std::uint32_t source,
    IStoryRecordArrayRuntime& runtime) {
    detail::InsertRecordCopies<36>(array, position, count, source,
                                   runtime);
}

void StoryRecordArray::FillCallback(
    std::uint32_t destination, std::uint32_t count,
    std::uint32_t source, IStoryRecordArrayRuntime& runtime) {
    for (std::uint32_t index = 0; index < count; ++index)
        runtime.CopyRecord(destination + index * 36, source);
}

std::uint32_t StoryRecordArray::Size(
    std::uint32_t array, IStoryRecordArrayRuntime& runtime) {
    return detail::RecordArraySize<36>(array, runtime);
}

void StoryRecordArray::InsertCopies(
    std::uint32_t array, std::uint32_t position,
    std::uint32_t count, std::uint32_t source,
    IStoryRecordArrayRuntime& runtime) {
    detail::InsertRecordCopies<36>(array, position, count, source, runtime);
}

void StoryRecordArray::Fill(
    std::uint32_t destination, std::uint32_t count,
    std::uint32_t source, IStoryRecordArrayRuntime& runtime) {
    for (std::uint32_t index = 0; index < count; ++index)
        runtime.CopyRecord(destination + index * 36, source);
}

}  // namespace pal4
