#include "pal4/magic_data_record_copy.h"

#include <initializer_list>

namespace pal4 {
namespace {

void AssignRecord(std::uint32_t destination, std::uint32_t source,
                  IGameDataRecordCopyRuntime& runtime) {
    for (const auto offset : {0u, 4u, 8u})
        runtime.WriteWord(destination + offset,
                          runtime.ReadWord(source + offset));
    for (const auto offset : {12u, 28u, 44u})
        static_cast<void>(runtime.AssignText(destination + offset,
                                             source + offset));
    for (std::uint32_t offset = 60; offset < 88; offset += 4)
        runtime.WriteWord(destination + offset,
                          runtime.ReadWord(source + offset));
    for (const auto offset : {88u, 104u, 120u, 136u})
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

std::uint32_t MagicDataRecordCopy::AssignRepeated(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t source, IGameDataRecordCopyRuntime& runtime) {
    for (auto destination = begin; destination != end;
         destination += kRecordBytes)
        AssignRecord(destination, source, runtime);
    return end;
}

std::uint32_t MagicDataRecordCopy::AssignBackward(
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

std::uint32_t MagicDataRecordCopy::Construct(
    std::uint32_t destination, std::uint32_t source,
    IGameDataRecordCopyRuntime& runtime) {
    if (!destination) return 0;
    for (const auto offset : {0u, 4u, 8u})
        runtime.WriteWord(destination + offset,
                          runtime.ReadWord(source + offset));
    for (const auto offset : {12u, 28u, 44u})
        InitializeText(destination + offset, source + offset, runtime);
    for (std::uint32_t offset = 60; offset < 88; offset += 4)
        runtime.WriteWord(destination + offset,
                          runtime.ReadWord(source + offset));
    for (const auto offset : {88u, 104u, 120u})
        InitializeText(destination + offset, source + offset, runtime);
    runtime.WriteByte(destination + 136, runtime.ReadByte(source + 136));
    for (const auto offset : {140u, 144u, 148u})
        runtime.WriteWord(destination + offset, 0);
    return runtime.AssignText(destination + 136, source + 136);
}

std::uint32_t MagicDataRecordCopy::Assign(
    std::uint32_t destination, std::uint32_t source,
    IGameDataRecordCopyRuntime& runtime) {
    AssignRecord(destination, source, runtime);
    return destination;
}

std::uint32_t MagicDataRecordCopy::CopyUnique(
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
