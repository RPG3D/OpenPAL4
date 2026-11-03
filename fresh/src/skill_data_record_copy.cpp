#include "pal4/skill_data_record_copy.h"

#include <initializer_list>

namespace pal4 {
namespace {

void AssignRecord(std::uint32_t destination, std::uint32_t source,
                  IGameDataRecordCopyRuntime& runtime) {
    for (std::uint32_t offset = 0; offset < 68; offset += 4)
        runtime.WriteWord(destination + offset,
                          runtime.ReadWord(source + offset));
    for (const auto offset : {68u, 84u, 100u, 116u})
        static_cast<void>(runtime.AssignText(destination + offset,
                                             source + offset));
}

}  // namespace

std::uint32_t SkillDataRecordCopy::AssignRepeated(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t source, IGameDataRecordCopyRuntime& runtime) {
    for (auto destination = begin; destination != end;
         destination += kRecordBytes)
        AssignRecord(destination, source, runtime);
    return end;
}

std::uint32_t SkillDataRecordCopy::AssignBackward(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination_end,
    IGameDataRecordCopyRuntime& runtime) {
    while (end != begin) {
        end -= kRecordBytes;
        destination_end -= kRecordBytes;
        AssignRecord(destination_end, end, runtime);
    }
    return destination_end;
}

std::uint32_t SkillDataRecordCopy::Construct(
    std::uint32_t destination, std::uint32_t source,
    IGameDataRecordCopyRuntime& runtime) {
    if (!destination) return 0;
    for (std::uint32_t offset = 0; offset < 20; offset += 4)
        runtime.WriteWord(destination + offset,
                          runtime.ReadWord(source + offset));
    runtime.CopyBytes(destination + 20, source + 20, 49);
    for (std::uint32_t offset : {72u, 76u, 80u})
        runtime.WriteWord(destination + offset, 0);
    static_cast<void>(runtime.AssignText(destination + 68, source + 68));

    runtime.WriteByte(destination + 84, runtime.ReadByte(source + 84));
    runtime.ResetText(destination + 84);
    static_cast<void>(runtime.AssignText(destination + 84, source + 84));

    std::uint32_t result = 0;
    for (const auto offset : {100u, 116u}) {
        runtime.WriteByte(destination + offset,
                          runtime.ReadByte(source + offset));
        for (std::uint32_t step : {4u, 8u, 12u})
            runtime.WriteWord(destination + offset + step, 0);
        result = runtime.AssignText(destination + offset,
                                    source + offset);
    }
    return result;
}

std::uint32_t SkillDataRecordCopy::Assign(
    std::uint32_t destination, std::uint32_t source,
    IGameDataRecordCopyRuntime& runtime) {
    AssignRecord(destination, source, runtime);
    return destination;
}

std::uint32_t SkillDataRecordCopy::CopyUnique(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination,
    const std::function<bool(std::uint32_t, std::uint32_t)>& equal,
    IGameDataRecordCopyRuntime& runtime) {
    if (begin == end) return destination;
    auto previous = begin;
    AssignRecord(destination, previous, runtime);
    destination += kRecordBytes;
    for (auto current = begin + kRecordBytes; current != end;
         current += kRecordBytes) {
        if (!equal(previous, current)) {
            AssignRecord(destination, current, runtime);
            previous = current;
            destination += kRecordBytes;
        }
    }
    return destination;
}

}  // namespace pal4
