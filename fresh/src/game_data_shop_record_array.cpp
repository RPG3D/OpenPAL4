#include "pal4/game_data_shop_record_array.h"

namespace pal4 {
namespace {

std::uint32_t CopyRange(std::uint32_t begin, std::uint32_t end,
                        std::uint32_t destination,
                        std::uint32_t record_bytes,
                        IGameDataShopRecordArrayRuntime& runtime) {
    for (auto source = begin; source != end;
         source += record_bytes, destination += record_bytes)
        runtime.CopyConstruct(destination, source, record_bytes);
    return destination;
}

std::uint32_t Insert(std::uint32_t array, std::uint32_t position,
                     std::uint32_t source, std::uint32_t record_bytes,
                     IGameDataShopRecordArrayRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    const auto end = runtime.ReadWord(array + 8);
    const auto capacity_end = runtime.ReadWord(array + 12);
    const auto index = (position - begin) / record_bytes;
    if ((capacity_end - end) / record_bytes != 0) {
        if ((end - position) / record_bytes != 0) {
            runtime.CopyConstruct(end, end - record_bytes, record_bytes);
            auto from = end - record_bytes;
            auto to = end;
            while (from != position) {
                from -= record_bytes;
                to -= record_bytes;
                runtime.Assign(to, from, record_bytes);
            }
            runtime.Assign(position, source, record_bytes);
        } else {
            runtime.CopyConstruct(end, source, record_bytes);
        }
        runtime.WriteWord(array + 8, end + record_bytes);
        return begin + index * record_bytes;
    }

    const auto used = begin ? (end - begin) / record_bytes : 0;
    const auto growth = used > 1 ? used : 1;
    const auto new_capacity = used + growth;
    const auto replacement = runtime.Allocate(new_capacity * record_bytes);
    auto destination = CopyRange(begin, position, replacement,
                                 record_bytes, runtime);
    runtime.CopyConstruct(destination, source, record_bytes);
    destination += record_bytes;
    static_cast<void>(CopyRange(position, end, destination,
                                record_bytes, runtime));
    runtime.DestroyRange(begin, end, record_bytes);
    runtime.Free(begin);
    runtime.WriteWord(array + 12,
                      replacement + new_capacity * record_bytes);
    runtime.WriteWord(array + 4, replacement);
    runtime.WriteWord(array + 8,
                      replacement + (used + 1) * record_bytes);
    return replacement + index * record_bytes;
}

void FillConstruct(std::uint32_t destination, std::uint32_t count,
                   std::uint32_t source, std::uint32_t record_bytes,
                   IGameDataShopRecordArrayRuntime& runtime) {
    for (std::uint32_t i = 0; i < count; ++i)
        runtime.CopyConstruct(destination + i * record_bytes, source,
                              record_bytes);
}

void AssignRepeated(std::uint32_t begin, std::uint32_t end,
                    std::uint32_t source, std::uint32_t record_bytes,
                    IGameDataShopRecordArrayRuntime& runtime) {
    for (auto destination = begin; destination != end;
         destination += record_bytes)
        runtime.Assign(destination, source, record_bytes);
}

void InsertCopies(std::uint32_t array, std::uint32_t position,
                  std::uint32_t count, std::uint32_t source,
                  std::uint32_t record_bytes,
                  IGameDataShopRecordArrayRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    const auto end = runtime.ReadWord(array + 8);
    const auto capacity_end = runtime.ReadWord(array + 12);
    const auto spare = (capacity_end - end) / record_bytes;
    if (spare >= count) {
        const auto tail = (end - position) / record_bytes;
        if (tail >= count) {
            if (count != 0) {
                static_cast<void>(CopyRange(end - count * record_bytes,
                    end, end, record_bytes, runtime));
                auto from = end - count * record_bytes;
                auto to = end;
                while (from != position) {
                    from -= record_bytes;
                    to -= record_bytes;
                    runtime.Assign(to, from, record_bytes);
                }
                AssignRepeated(position, position + count * record_bytes,
                               source, record_bytes, runtime);
                runtime.WriteWord(array + 8,
                                  end + count * record_bytes);
            }
        } else {
            static_cast<void>(CopyRange(position, end,
                position + count * record_bytes, record_bytes, runtime));
            FillConstruct(end, count - tail, source, record_bytes, runtime);
            AssignRepeated(position, end, source, record_bytes, runtime);
            runtime.WriteWord(array + 8,
                              end + count * record_bytes);
        }
        return;
    }

    const auto used = begin ? (end - begin) / record_bytes : 0;
    const auto growth = used > count ? used : count;
    const auto replacement = runtime.Allocate(
        (used + growth) * record_bytes);
    auto destination = CopyRange(begin, position, replacement,
                                 record_bytes, runtime);
    FillConstruct(destination, count, source, record_bytes, runtime);
    destination += count * record_bytes;
    static_cast<void>(CopyRange(position, end, destination,
                                record_bytes, runtime));
    runtime.DestroyRange(begin, end, record_bytes);
    runtime.Free(begin);
    runtime.WriteWord(array + 12,
                      replacement + (used + growth) * record_bytes);
    runtime.WriteWord(array + 4, replacement);
    runtime.WriteWord(array + 8,
                      replacement + (used + count) * record_bytes);
}

}  // namespace

std::uint32_t GameDataShopRecordArray::InsertProperty(
    std::uint32_t array, std::uint32_t position, std::uint32_t source,
    IGameDataShopRecordArrayRuntime& runtime) {
    return Insert(array, position, source, kPropertyBytes, runtime);
}

std::uint32_t GameDataShopRecordArray::SizeProperty(
    std::uint32_t array, IGameDataShopRecordArrayRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    return begin ?
        (runtime.ReadWord(array + 8) - begin) / kPropertyBytes : 0;
}

std::uint32_t GameDataShopRecordArray::CopyPropertyRange(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination,
    IGameDataShopRecordArrayRuntime& runtime) {
    return CopyRange(begin, end, destination, kPropertyBytes, runtime);
}

std::uint32_t GameDataShopRecordArray::InsertGoods(
    std::uint32_t array, std::uint32_t position, std::uint32_t source,
    IGameDataShopRecordArrayRuntime& runtime) {
    return Insert(array, position, source, kGoodsBytes, runtime);
}

std::uint32_t GameDataShopRecordArray::CopyGoodsRange(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination,
    IGameDataShopRecordArrayRuntime& runtime) {
    return CopyRange(begin, end, destination, kGoodsBytes, runtime);
}

void GameDataShopRecordArray::InsertPropertyCopies(
    std::uint32_t array, std::uint32_t position,
    std::uint32_t count, std::uint32_t source,
    IGameDataShopRecordArrayRuntime& runtime) {
    InsertCopies(array, position, count, source, kPropertyBytes, runtime);
}

void GameDataShopRecordArray::InsertGoodsCopies(
    std::uint32_t array, std::uint32_t position,
    std::uint32_t count, std::uint32_t source,
    IGameDataShopRecordArrayRuntime& runtime) {
    InsertCopies(array, position, count, source, kGoodsBytes, runtime);
}

void GameDataShopRecordArray::FillProperty(
    std::uint32_t destination, std::uint32_t count,
    std::uint32_t source, IGameDataShopRecordArrayRuntime& runtime) {
    FillConstruct(destination, count, source, kPropertyBytes, runtime);
}

void GameDataShopRecordArray::FillGoods(
    std::uint32_t destination, std::uint32_t count,
    std::uint32_t source, IGameDataShopRecordArrayRuntime& runtime) {
    FillConstruct(destination, count, source, kGoodsBytes, runtime);
}

}  // namespace pal4
