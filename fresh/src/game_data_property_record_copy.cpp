#include "pal4/game_data_property_record_copy.h"

namespace pal4 {
namespace {

constexpr std::uint32_t kRecordBytes = 168;

void CopyFields(std::uint32_t destination, std::uint32_t source,
                bool constructing,
                IGameDataPropertyRecordCopyRuntime& runtime) {
    for (std::uint32_t offset = 0; offset < 16; offset += 4)
        runtime.WriteWord(destination + offset,
                          runtime.ReadWord(source + offset));
    for (std::uint32_t i = 0; i < 7; ++i) {
        const auto offset = 16 + i * 16;
        if (constructing)
            runtime.ConstructText(destination + offset, source + offset);
        else
            runtime.AssignText(destination + offset, source + offset);
    }
    for (std::uint32_t offset = 128; offset < kRecordBytes; offset += 4)
        runtime.WriteWord(destination + offset,
                          runtime.ReadWord(source + offset));
}

}  // namespace

std::uint32_t GameDataPropertyRecordCopy::AssignRange(
    std::uint32_t begin, std::uint32_t end, std::uint32_t source,
    IGameDataPropertyRecordCopyRuntime& runtime) {
    for (auto destination = begin; destination != end;
         destination += kRecordBytes)
        CopyFields(destination, source, false, runtime);
    return end;
}

std::uint32_t GameDataPropertyRecordCopy::AssignBackward(
    std::uint32_t source_begin, std::uint32_t source_end,
    std::uint32_t destination_end,
    IGameDataPropertyRecordCopyRuntime& runtime) {
    while (source_end != source_begin) {
        source_end -= kRecordBytes;
        destination_end -= kRecordBytes;
        CopyFields(destination_end, source_end, false, runtime);
    }
    return destination_end;
}

std::uint32_t GameDataPropertyRecordCopy::Construct(
    std::uint32_t destination, std::uint32_t source,
    IGameDataPropertyRecordCopyRuntime& runtime) {
    if (!destination) return 0;
    CopyFields(destination, source, true, runtime);
    return source;
}

std::uint32_t GameDataPropertyRecordCopy::Assign(
    std::uint32_t destination, std::uint32_t source,
    IGameDataPropertyRecordCopyRuntime& runtime) {
    CopyFields(destination, source, false, runtime);
    return destination;
}

std::uint32_t GameDataPropertyRecordCopy::UniqueCopy(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination,
    IGameDataPropertyRecordCopyRuntime& runtime) {
    if (begin == end) return destination;
    (void)Assign(destination, begin, runtime);
    auto last_retained = begin;
    destination += kRecordBytes;
    for (auto source = begin + kRecordBytes; source != end;
         source += kRecordBytes) {
        if (runtime.ReadWord(last_retained) == runtime.ReadWord(source))
            continue;
        (void)Assign(destination, source, runtime);
        destination += kRecordBytes;
        last_retained = source;
    }
    return destination;
}

}  // namespace pal4
