#include "pal4/game_data_parameter_record_copy.h"

namespace pal4 {
namespace {

constexpr std::uint32_t kRecordBytes = 328;
constexpr std::uint32_t kTextOffset = 4;
constexpr std::uint32_t kTextBytes = 16;
constexpr std::uint32_t kTextCount = 6;
constexpr std::uint32_t kScalarTailOffset = 100;

}  // namespace

void GameDataParameterRecordCopy::Assign(
    std::uint32_t destination, std::uint32_t source,
    IGameDataParameterRecordCopyRuntime& runtime) {
    runtime.CopyBytes(destination, source, 4);
    for (std::uint32_t i = 0; i < kTextCount; ++i) {
        const auto offset = kTextOffset + i * kTextBytes;
        runtime.AssignText(destination + offset, source + offset);
    }
    runtime.CopyBytes(destination + kScalarTailOffset,
                      source + kScalarTailOffset,
                      kRecordBytes - kScalarTailOffset);
}

std::uint32_t GameDataParameterRecordCopy::AssignRange(
    std::uint32_t begin, std::uint32_t end, std::uint32_t source,
    IGameDataParameterRecordCopyRuntime& runtime) {
    for (auto destination = begin; destination != end;
         destination += kRecordBytes)
        Assign(destination, source, runtime);
    return end;
}

std::uint32_t GameDataParameterRecordCopy::AssignBackward(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination_end,
    IGameDataParameterRecordCopyRuntime& runtime) {
    while (end != begin) {
        end -= kRecordBytes;
        destination_end -= kRecordBytes;
        Assign(destination_end, end, runtime);
    }
    return destination_end;
}

void GameDataParameterRecordCopy::Construct(
    std::uint32_t destination, std::uint32_t source,
    IGameDataParameterRecordCopyRuntime& runtime) {
    if (!destination) return;
    runtime.CopyBytes(destination, source, 4);
    for (std::uint32_t i = 0; i < kTextCount; ++i) {
        const auto offset = kTextOffset + i * kTextBytes;
        runtime.WriteByte(destination + offset,
                          runtime.ReadByte(source + offset));
        runtime.WriteWord(destination + offset + 4, 0);
        runtime.WriteWord(destination + offset + 8, 0);
        runtime.WriteWord(destination + offset + 12, 0);
        runtime.AssignText(destination + offset, source + offset);
    }
    runtime.CopyBytes(destination + kScalarTailOffset,
                      source + kScalarTailOffset,
                      kRecordBytes - kScalarTailOffset);
}

std::uint32_t GameDataParameterRecordCopy::AssignRecord(
    std::uint32_t destination, std::uint32_t source,
    IGameDataParameterRecordCopyRuntime& runtime) {
    Assign(destination, source, runtime);
    return destination;
}

std::uint32_t GameDataParameterRecordCopy::CopySelected(
    std::uint32_t begin, std::uint32_t end, std::uint32_t destination,
    const std::function<bool(std::uint32_t, std::uint32_t)>& equivalent,
    IGameDataParameterRecordCopyRuntime& runtime) {
    if (begin == end) return destination;
    Assign(destination, begin, runtime);
    auto last_retained = begin;
    destination += kRecordBytes;
    for (auto current = begin + kRecordBytes; current != end;
         current += kRecordBytes) {
        if (!equivalent(last_retained, current)) {
            Assign(destination, current, runtime);
            last_retained = current;
            destination += kRecordBytes;
        }
    }
    return destination;
}

}  // namespace pal4
