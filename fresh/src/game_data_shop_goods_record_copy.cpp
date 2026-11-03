#include "pal4/game_data_shop_goods_record_copy.h"

namespace pal4 {
namespace {

void AssignRecord(std::uint32_t destination, std::uint32_t source,
                  IGameDataShopGoodsRecordCopyRuntime& runtime) {
    for (std::uint32_t offset = 0; offset < 20; offset += 4)
        runtime.WriteWord(destination + offset,
                          runtime.ReadWord(source + offset));
    runtime.AssignText(destination + 20, source + 20);
}

}  // namespace

void GameDataShopGoodsRecordCopy::Construct(
    std::uint32_t destination, std::uint32_t source,
    IGameDataShopGoodsRecordCopyRuntime& runtime) {
    if (!destination) return;
    for (std::uint32_t offset = 0; offset < 20; offset += 4)
        runtime.WriteWord(destination + offset,
                          runtime.ReadWord(source + offset));
    runtime.WriteByte(destination + 20, runtime.ReadByte(source + 20));
    runtime.WriteWord(destination + 24, 0);
    runtime.WriteWord(destination + 28, 0);
    runtime.WriteWord(destination + 32, 0);
    runtime.AssignText(destination + 20, source + 20);
}

std::uint32_t GameDataShopGoodsRecordCopy::Assign(
    std::uint32_t destination, std::uint32_t source,
    IGameDataShopGoodsRecordCopyRuntime& runtime) {
    AssignRecord(destination, source, runtime);
    return destination;
}

std::uint32_t GameDataShopGoodsRecordCopy::AssignRepeated(
    std::uint32_t begin, std::uint32_t end, std::uint32_t source,
    IGameDataShopGoodsRecordCopyRuntime& runtime) {
    for (auto destination = begin; destination != end;
         destination += kRecordBytes)
        AssignRecord(destination, source, runtime);
    return end;
}

std::uint32_t GameDataShopGoodsRecordCopy::AssignBackward(
    std::uint32_t begin, std::uint32_t end, std::uint32_t destination_end,
    IGameDataShopGoodsRecordCopyRuntime& runtime) {
    while (end != begin) {
        end -= kRecordBytes;
        destination_end -= kRecordBytes;
        AssignRecord(destination_end, end, runtime);
    }
    return destination_end;
}

std::uint32_t GameDataShopGoodsRecordCopy::CopyUnique(
    std::uint32_t begin, std::uint32_t end, std::uint32_t destination,
    IGameDataShopGoodsRecordCopyRuntime& runtime) {
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

}  // namespace pal4
