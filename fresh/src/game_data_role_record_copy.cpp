#include "pal4/game_data_role_record_copy.h"

namespace pal4 {
namespace {

void AssignTexts(std::uint32_t destination, std::uint32_t source,
                 std::uint32_t start, std::uint32_t count,
                 IGameDataRoleRecordCopyRuntime& runtime) {
    for (std::uint32_t i = 0; i < count; ++i)
        static_cast<void>(runtime.AssignText(
            destination + start + i * 16,
            source + start + i * 16));
}

void ConstructTexts(std::uint32_t destination, std::uint32_t source,
                    std::uint32_t start, std::uint32_t count,
                    IGameDataRoleRecordCopyRuntime& runtime,
                    std::uint32_t& result) {
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto offset = start + i * 16;
        runtime.WriteByte(destination + offset,
                          runtime.ReadByte(source + offset));
        runtime.WriteWord(destination + offset + 4, 0);
        runtime.WriteWord(destination + offset + 8, 0);
        runtime.WriteWord(destination + offset + 12, 0);
        result = runtime.AssignText(destination + offset, source + offset);
    }
}

}  // namespace

void GameDataRoleRecordCopy::AssignRecord(
    std::uint32_t destination, std::uint32_t source,
    IGameDataRoleRecordCopyRuntime& runtime) {
    runtime.CopyBytes(destination, source, 144);
    static_cast<void>(AssignTail(destination + 144, source + 144, runtime));
}

std::uint32_t GameDataRoleRecordCopy::AssignRange(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t source, IGameDataRoleRecordCopyRuntime& runtime) {
    for (auto destination = begin; destination != end;
         destination += kRecordBytes)
        AssignRecord(destination, source, runtime);
    return end;
}

std::uint32_t GameDataRoleRecordCopy::AssignBackward(
    std::uint32_t begin, std::uint32_t source_end,
    std::uint32_t destination_end,
    IGameDataRoleRecordCopyRuntime& runtime) {
    while (source_end != begin) {
        source_end -= kRecordBytes;
        destination_end -= kRecordBytes;
        AssignRecord(destination_end, source_end, runtime);
    }
    return destination_end;
}

std::uint32_t GameDataRoleRecordCopy::CopyConstruct(
    std::uint32_t destination, std::uint32_t source,
    IGameDataRoleRecordCopyRuntime& runtime) {
    if (!destination) return 0;
    runtime.CopyBytes(destination, source, 144);
    std::uint32_t result = destination;
    ConstructTexts(destination, source, 144, 4, runtime, result);
    runtime.CopyBytes(destination + 208, source + 208, 32);
    ConstructTexts(destination, source, 240, 5, runtime, result);
    return result;
}

std::uint32_t GameDataRoleRecordCopy::AssignOne(
    std::uint32_t destination, std::uint32_t source,
    IGameDataRoleRecordCopyRuntime& runtime) {
    AssignRecord(destination, source, runtime);
    return destination;
}

std::uint32_t GameDataRoleRecordCopy::AssignTail(
    std::uint32_t destination, std::uint32_t source,
    IGameDataRoleRecordCopyRuntime& runtime) {
    AssignTexts(destination, source, 0, 4, runtime);
    runtime.CopyBytes(destination + 64, source + 64, 32);
    AssignTexts(destination, source, 96, 5, runtime);
    return destination;
}

std::uint32_t GameDataRoleRecordCopy::UniqueCopy(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination,
    const std::function<bool(std::uint32_t, std::uint32_t)>& equivalent,
    IGameDataRoleRecordCopyRuntime& runtime) {
    if (begin == end) return destination;
    static_cast<void>(AssignOne(destination, begin, runtime));
    auto last_retained = begin;
    destination += kRecordBytes;
    for (auto source = begin + kRecordBytes; source != end;
         source += kRecordBytes) {
        if (equivalent(last_retained, source)) continue;
        static_cast<void>(AssignOne(destination, source, runtime));
        destination += kRecordBytes;
        last_retained = source;
    }
    return destination;
}

}  // namespace pal4
