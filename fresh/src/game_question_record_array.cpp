#include "pal4/game_question_record_array.h"
#include "record_array_common.h"

namespace pal4 {

std::uint32_t GameQuestionRecordArray::Insert(
    std::uint32_t array, std::uint32_t position,
    std::uint32_t source, IGameQuestionRecordArrayRuntime& runtime) {
    return detail::InsertRecord<76>(array, position, source, runtime);
}

std::uint32_t GameQuestionRecordArray::Size(
    std::uint32_t array, IGameQuestionRecordArrayRuntime& runtime) {
    return detail::RecordArraySize<76>(array, runtime);
}

std::uint32_t GameQuestionRecordArray::CopyRange(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination, IGameQuestionRecordArrayRuntime& runtime) {
    return detail::CopyRecordRange<76>(begin, end, destination, runtime);
}

void GameQuestionRecordArray::InsertCopies(
    std::uint32_t array, std::uint32_t position,
    std::uint32_t count, std::uint32_t source,
    IGameQuestionRecordArrayRuntime& runtime) {
    detail::InsertRecordCopies<76>(array, position, count, source, runtime);
}

std::uint32_t GameQuestionRecordArray::Fill(
    std::uint32_t destination, std::uint32_t count,
    std::uint32_t source, IGameQuestionRecordArrayRuntime& runtime) {
    std::uint32_t result = 0;
    for (std::uint32_t index = 0; index < count; ++index)
        result = runtime.CopyRecord(destination + index * 76, source);
    return result;
}

}  // namespace pal4
