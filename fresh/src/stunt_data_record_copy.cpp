#include "pal4/stunt_data_record_copy.h"

#include <initializer_list>

namespace pal4 {
namespace {

void AssignRecord(std::uint32_t destination, std::uint32_t source,
                  IGameDataRecordCopyRuntime& runtime) {
    for (const auto offset : {0u, 4u})
        runtime.WriteWord(destination + offset,
                          runtime.ReadWord(source + offset));
    for (const auto offset : {8u, 24u, 40u})
        static_cast<void>(runtime.AssignText(destination + offset,
                                             source + offset));
    for (std::uint32_t offset = 56; offset < 144; offset += 4)
        runtime.WriteWord(destination + offset,
                          runtime.ReadWord(source + offset));
    for (const auto offset : {144u, 160u, 176u, 192u})
        static_cast<void>(runtime.AssignText(destination + offset,
                                             source + offset));
}

void InitializeText(std::uint32_t destination, std::uint32_t source,
                    IGameDataRecordCopyRuntime& runtime) {
    runtime.WriteByte(destination, runtime.ReadByte(source));
    runtime.ResetText(destination);
    static_cast<void>(runtime.AssignText(destination, source));
}

}  // namespace

std::uint32_t StuntDataRecordCopy::AssignRepeated(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t source, IGameDataRecordCopyRuntime& runtime) {
    for (auto destination = begin; destination != end;
         destination += kRecordBytes)
        AssignRecord(destination, source, runtime);
    return end;
}

std::uint32_t StuntDataRecordCopy::AssignBackward(
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

std::uint32_t StuntDataRecordCopy::Construct(
    std::uint32_t destination, std::uint32_t source,
    IGameDataRecordCopyRuntime& runtime) {
    if (!destination) return 0;
    for (const auto offset : {0u, 4u})
        runtime.WriteWord(destination + offset,
                          runtime.ReadWord(source + offset));
    for (const auto offset : {8u, 24u, 40u})
        InitializeText(destination + offset, source + offset, runtime);
    for (std::uint32_t offset = 56; offset < 72; offset += 4)
        runtime.WriteWord(destination + offset,
                          runtime.ReadWord(source + offset));
    runtime.CopyBytes(destination + 72, source + 72, 20);
    runtime.CopyBytes(destination + 92, source + 92, 53);
    runtime.ResetText(destination + 144);
    static_cast<void>(runtime.AssignText(destination + 144,
                                         source + 144));
    std::uint32_t result = 0;
    for (const auto offset : {160u, 176u, 192u}) {
        runtime.WriteByte(destination + offset,
                          runtime.ReadByte(source + offset));
        runtime.ResetText(destination + offset);
        result = runtime.AssignText(destination + offset,
                                    source + offset);
    }
    return result;
}

std::uint32_t StuntDataRecordCopy::Assign(
    std::uint32_t destination, std::uint32_t source,
    IGameDataRecordCopyRuntime& runtime) {
    AssignRecord(destination, source, runtime);
    return destination;
}

std::uint32_t StuntDataRecordCopy::CopyUnique(
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
