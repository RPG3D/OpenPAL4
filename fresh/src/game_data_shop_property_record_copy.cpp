#include "pal4/game_data_shop_property_record_copy.h"

#include <initializer_list>

namespace pal4 {
namespace {

void AssignRecord(std::uint32_t destination, std::uint32_t source,
                  IGameDataShopPropertyRecordCopyRuntime& runtime) {
    runtime.WriteWord(destination, runtime.ReadWord(source));
    runtime.AssignText(destination + 4, source + 4);
    runtime.AssignText(destination + 20, source + 20);
    runtime.AssignText(destination + 36, source + 36);
    runtime.WriteWord(destination + 52, runtime.ReadWord(source + 52));
}

}  // namespace

void GameDataShopPropertyRecordCopy::Construct(
    std::uint32_t destination, std::uint32_t source,
    IGameDataShopPropertyRecordCopyRuntime& runtime) {
    if (!destination) return;
    runtime.WriteWord(destination, runtime.ReadWord(source));
    for (std::uint32_t offset : {4u, 20u, 36u}) {
        runtime.WriteWord(destination + offset + 4, 0);
        runtime.WriteByte(destination + offset,
                          runtime.ReadByte(source + offset));
        runtime.WriteWord(destination + offset + 8, 0);
        runtime.WriteWord(destination + offset + 12, 0);
        runtime.AssignText(destination + offset, source + offset);
    }
    runtime.WriteWord(destination + 52, runtime.ReadWord(source + 52));
}

std::uint32_t GameDataShopPropertyRecordCopy::Assign(
    std::uint32_t destination, std::uint32_t source,
    IGameDataShopPropertyRecordCopyRuntime& runtime) {
    AssignRecord(destination, source, runtime);
    return destination;
}

std::uint32_t GameDataShopPropertyRecordCopy::CopyUnique(
    std::uint32_t begin, std::uint32_t end, std::uint32_t destination,
    IGameDataShopPropertyRecordCopyRuntime& runtime) {
    if (begin == end) return destination;
    auto previous = begin;
    AssignRecord(destination, previous, runtime);
    destination += kRecordBytes;
    for (auto current = begin + kRecordBytes; current != end;
         current += kRecordBytes) {
        if (!runtime.RecordsEqual(previous, current)) {
            AssignRecord(destination, current, runtime);
            destination += kRecordBytes;
            previous = current;
        }
    }
    return destination;
}

std::uint32_t GameDataShopPropertyRecordCopy::AssignRepeated(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t source,
    IGameDataShopPropertyRecordCopyRuntime& runtime) {
    for (auto destination = begin; destination != end;
         destination += kRecordBytes)
        AssignRecord(destination, source, runtime);
    return end;
}

std::uint32_t GameDataShopPropertyRecordCopy::AssignBackward(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination_end,
    IGameDataShopPropertyRecordCopyRuntime& runtime) {
    while (end != begin) {
        end -= kRecordBytes;
        destination_end -= kRecordBytes;
        AssignRecord(destination_end, end, runtime);
    }
    return destination_end;
}

}  // namespace pal4
