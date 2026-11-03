#include "pal4/game_question_record_copy.h"

#include <initializer_list>

namespace pal4 {
namespace {

void AssignOne(std::uint32_t destination, std::uint32_t source,
               IGameQuestionRecordCopyRuntime& runtime) {
    runtime.WriteWord(destination, runtime.ReadWord(source));
    for (const auto offset : {4u, 20u, 36u, 52u})
        static_cast<void>(runtime.AssignText(destination + offset,
                                              source + offset));
    runtime.WriteWord(destination + 68, runtime.ReadWord(source + 68));
    runtime.WriteWord(destination + 72, runtime.ReadWord(source + 72));
}

}  // namespace

std::uint32_t GameQuestionRecordCopy::Assign(
    std::uint32_t destination, std::uint32_t source,
    IGameQuestionRecordCopyRuntime& runtime) {
    AssignOne(destination, source, runtime);
    return destination;
}

std::uint32_t GameQuestionRecordCopy::Construct(
    std::uint32_t destination, std::uint32_t source,
    IGameQuestionRecordCopyRuntime& runtime) {
    if (!destination) return 0;
    runtime.WriteWord(destination, runtime.ReadWord(source));
    for (const auto offset : {4u, 20u, 36u}) {
        runtime.WriteByte(destination + offset,
                          runtime.ReadByte(source + offset));
        runtime.WriteWord(destination + offset + 4, 0);
        runtime.WriteWord(destination + offset + 8, 0);
        runtime.WriteWord(destination + offset + 12, 0);
        static_cast<void>(runtime.AssignText(destination + offset,
                                              source + offset));
    }
    runtime.WriteByte(destination + 52, runtime.ReadByte(source + 52));
    runtime.SetErrorFlag(destination + 52, 0);
    const auto result = runtime.AssignText(destination + 52, source + 52);
    runtime.WriteWord(destination + 68, runtime.ReadWord(source + 68));
    runtime.WriteWord(destination + 72, runtime.ReadWord(source + 72));
    return result;
}

std::uint32_t GameQuestionRecordCopy::AssignRepeated(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t source, IGameQuestionRecordCopyRuntime& runtime) {
    for (auto destination = begin; destination != end; destination += 76)
        AssignOne(destination, source, runtime);
    return end;
}

std::uint32_t GameQuestionRecordCopy::AssignBackward(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination_end,
    IGameQuestionRecordCopyRuntime& runtime) {
    while (end != begin) {
        end -= 76;
        destination_end -= 76;
        AssignOne(destination_end, end, runtime);
    }
    return destination_end;
}

std::uint32_t GameQuestionRecordCopy::CopyUnique(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination,
    IGameQuestionRecordCopyRuntime& runtime) {
    if (begin == end) return destination;
    auto previous = begin;
    AssignOne(destination, previous, runtime);
    destination += 76;
    for (auto current = begin + 76; current != end; current += 76) {
        if (!runtime.RecordsEqual(previous, current)) {
            AssignOne(destination, current, runtime);
            destination += 76;
            previous = current;
        }
    }
    return destination;
}

}  // namespace pal4
