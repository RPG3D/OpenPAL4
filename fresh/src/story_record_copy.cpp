#include "pal4/story_record_copy.h"

#include <initializer_list>

namespace pal4 {
namespace {

void AssignOne(std::uint32_t destination, std::uint32_t source,
               IStoryRecordCopyRuntime& runtime) {
    runtime.WriteWord(destination, runtime.ReadWord(source));
    runtime.AssignText(destination + 4, source + 4);
    runtime.AssignText(destination + 20, source + 20);
}

}  // namespace

std::uint32_t StoryRecordCopy::Assign(
    std::uint32_t destination, std::uint32_t source,
    IStoryRecordCopyRuntime& runtime) {
    AssignOne(destination, source, runtime);
    return destination;
}

void StoryRecordCopy::Construct(
    std::uint32_t destination, std::uint32_t source,
    IStoryRecordCopyRuntime& runtime) {
    if (!destination) return;
    runtime.WriteWord(destination, runtime.ReadWord(source));
    for (const auto offset : {4u, 20u}) {
        runtime.WriteByte(destination + offset,
                          runtime.ReadByte(source + offset));
        runtime.WriteWord(destination + offset + 4, 0);
        runtime.WriteWord(destination + offset + 8, 0);
        runtime.WriteWord(destination + offset + 12, 0);
        runtime.AssignText(destination + offset, source + offset);
    }
}

void StoryRecordCopy::ConstructCallback(
    const std::uint32_t destination, const std::uint32_t source,
    IUtilitiesRecordTextRuntime& runtime) {
    if (destination == 0u) return;
    runtime.WriteWord(destination, runtime.ReadWord(source));

    runtime.WriteByte(destination + 4u, runtime.ReadByte(source + 4u));
    UtilitiesRecordTextField::Write(destination + 4u, {}, runtime);
    UtilitiesRecordTextField::Assign(destination + 4u,
                                     source + 4u, runtime);

    runtime.WriteByte(destination + 20u, runtime.ReadByte(source + 20u));
    UtilitiesRecordTextField::Write(destination + 20u, {}, runtime);
    UtilitiesRecordTextField::AssignInline(destination + 20u,
                                           source + 20u, runtime, true);
}

std::uint32_t StoryRecordCopy::AssignRepeated(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t source, IStoryRecordCopyRuntime& runtime) {
    for (auto destination = begin; destination != end; destination += 36)
        AssignOne(destination, source, runtime);
    return end;
}

std::uint32_t StoryRecordCopy::AssignBackward(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination_end, IStoryRecordCopyRuntime& runtime) {
    while (end != begin) {
        end -= 36;
        destination_end -= 36;
        AssignOne(destination_end, end, runtime);
    }
    return destination_end;
}

std::uint32_t StoryRecordCopy::CopyUnique(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination, IStoryRecordCopyRuntime& runtime) {
    if (begin == end) return destination;
    auto previous = begin;
    AssignOne(destination, previous, runtime);
    destination += 36;
    for (auto current = begin + 36; current != end; current += 36) {
        if (!runtime.RecordsEqual(previous, current)) {
            AssignOne(destination, current, runtime);
            destination += 36;
            previous = current;
        }
    }
    return destination;
}

}  // namespace pal4
