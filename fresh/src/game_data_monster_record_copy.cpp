#include "pal4/game_data_monster_record_copy.h"

#include <initializer_list>

namespace pal4 {
namespace {

constexpr std::uint32_t kRecordBytes = 596;
constexpr std::uint32_t kSubrecordOffset = 336;

void AssignTextMembers(std::uint32_t destination, std::uint32_t source,
                       IGameDataMonsterRecordCopyRuntime& runtime) {
    for (std::uint32_t i = 0; i < 6; ++i)
        runtime.AssignText(destination + 72 + i * 16,
                           source + 72 + i * 16);
    for (std::uint32_t i = 0; i < 5; ++i)
        runtime.AssignText(destination + 168 + i * 16,
                           source + 168 + i * 16);
}

void AssignSubrecordFields(std::uint32_t destination, std::uint32_t source,
                           IGameDataMonsterRecordCopyRuntime& runtime) {
    runtime.CopyBytes(destination, source, 72);
    AssignTextMembers(destination, source, runtime);
    runtime.CopyBytes(destination + 248, source + 248, 12);
}

}  // namespace

std::uint32_t GameDataMonsterRecordCopy::AssignRange(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t source,
    IGameDataMonsterRecordCopyRuntime& runtime) {
    for (auto destination = begin; destination != end;
         destination += kRecordBytes)
        Assign(destination, source, runtime);
    return end;
}

std::uint32_t GameDataMonsterRecordCopy::AssignBackward(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination_end,
    IGameDataMonsterRecordCopyRuntime& runtime) {
    while (end != begin) {
        end -= kRecordBytes;
        destination_end -= kRecordBytes;
        Assign(destination_end, end, runtime);
    }
    return destination_end;
}

std::uint32_t GameDataMonsterRecordCopy::Construct(
    std::uint32_t destination, std::uint32_t source,
    IGameDataMonsterRecordCopyRuntime& runtime) {
    if (!destination) return 0;
    runtime.CopyBytes(destination, source, kSubrecordOffset);
    runtime.CopyBytes(destination + kSubrecordOffset,
                      source + kSubrecordOffset, 72);
    for (std::uint32_t i = 0; i < 6; ++i) {
        const auto at = kSubrecordOffset + 72 + i * 16;
        runtime.WriteByte(destination + at, runtime.ReadByte(source + at));
        for (std::uint32_t step : {4u, 8u, 12u})
            runtime.WriteWord(destination + at + step, 0);
        runtime.AssignText(destination + at, source + at);
    }
    for (std::uint32_t i = 0; i < 5; ++i) {
        const auto at = kSubrecordOffset + 168 + i * 16;
        runtime.WriteByte(destination + at, runtime.ReadByte(source + at));
        for (std::uint32_t step : {4u, 8u, 12u})
            runtime.WriteWord(destination + at + step, 0);
        runtime.AssignText(destination + at, source + at);
    }
    runtime.CopyBytes(destination + 584, source + 584, 12);
    return runtime.ReadWord(source + 584);
}

void GameDataMonsterRecordCopy::Assign(
    std::uint32_t destination, std::uint32_t source,
    IGameDataMonsterRecordCopyRuntime& runtime) {
    runtime.CopyBytes(destination, source, kSubrecordOffset);
    AssignSubrecordFields(destination + kSubrecordOffset,
                          source + kSubrecordOffset, runtime);
}

std::uint32_t GameDataMonsterRecordCopy::AssignRecord(
    std::uint32_t destination, std::uint32_t source,
    IGameDataMonsterRecordCopyRuntime& runtime) {
    Assign(destination, source, runtime);
    return destination;
}

std::uint32_t GameDataMonsterRecordCopy::AssignSubrecord(
    std::uint32_t destination, std::uint32_t source,
    IGameDataMonsterRecordCopyRuntime& runtime) {
    AssignSubrecordFields(destination, source, runtime);
    return destination;
}

std::uint32_t GameDataMonsterRecordCopy::CopySelected(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination,
    const std::function<bool(std::uint32_t, std::uint32_t)>& equivalent,
    IGameDataMonsterRecordCopyRuntime& runtime) {
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
