#include "pal4/game_data_shop_storage_capacity.h"

namespace pal4 {
namespace {

std::uint32_t ReserveWithRecordBytes(
    std::uint32_t object, std::int32_t requested,
    std::uint32_t record_bytes,
    IGameDataShopStorageCapacityRuntime& runtime) {
    const auto requested_bits = static_cast<std::uint32_t>(requested);
    const auto record_begin = runtime.ReadWord(object + 96);
    const auto record_end = runtime.ReadWord(object + 100);
    const auto record_capacity_end = runtime.ReadWord(object + 104);
    const auto record_capacity = record_begin
        ? (record_capacity_end - record_begin) / record_bytes : 0;

    if (record_capacity < requested_bits) {
        const auto allocation_count = requested < 0 ? 0u : requested_bits;
        const auto new_begin = runtime.Allocate(allocation_count * record_bytes);
        auto destination = new_begin;
        for (auto source = record_begin; source != record_end;
             source += record_bytes, destination += record_bytes)
            runtime.CopyRecord(destination, source);
        for (auto source = record_begin; source != record_end;
             source += record_bytes)
            runtime.DestroyRecord(source);
        runtime.Free(record_begin);
        const auto active_count = record_begin
            ? (record_end - record_begin) / record_bytes : 0;
        runtime.WriteWord(object + 104,
                          new_begin + requested_bits * record_bytes);
        runtime.WriteWord(object + 96, new_begin);
        runtime.WriteWord(object + 100,
                          new_begin + active_count * record_bytes);
    }

    const auto index_begin = runtime.ReadWord(object + 8);
    const auto index_end = runtime.ReadWord(object + 12);
    const auto index_capacity_end = runtime.ReadWord(object + 16);
    const auto index_capacity = index_begin
        ? (index_capacity_end - index_begin) /
              GameDataShopStorageCapacity::kIndexBytes : 0;
    if (index_capacity >= requested_bits) return index_capacity;

    const auto allocation_count = requested < 0 ? 0u : requested_bits;
    const auto new_begin = runtime.Allocate(
        allocation_count * GameDataShopStorageCapacity::kIndexBytes);
    auto destination = new_begin;
    for (auto source = index_begin; source != index_end;
         source += GameDataShopStorageCapacity::kIndexBytes,
         destination += GameDataShopStorageCapacity::kIndexBytes) {
        if (destination) runtime.CopyIndex(destination, source);
    }
    runtime.Free(index_begin);
    runtime.WriteWord(object + 16,
                      new_begin + requested_bits *
                          GameDataShopStorageCapacity::kIndexBytes);
    runtime.WriteWord(object + 8, new_begin);
    const auto new_end = new_begin +
        (index_begin ? (index_end - index_begin) /
                           GameDataShopStorageCapacity::kIndexBytes : 0) *
            GameDataShopStorageCapacity::kIndexBytes;
    runtime.WriteWord(object + 12, new_end);
    return new_end;
}

}  // namespace

std::uint32_t GameDataShopStorageCapacity::Reserve(
    std::uint32_t object, std::int32_t requested,
    IGameDataShopStorageCapacityRuntime& runtime) {
    return ReserveWithRecordBytes(object, requested, kRecordBytes, runtime);
}

std::uint32_t GameDataShopStorageCapacity::ReserveGoods(
    std::uint32_t object, std::int32_t requested,
    IGameDataShopStorageCapacityRuntime& runtime) {
    return ReserveWithRecordBytes(object, requested, 36, runtime);
}

}  // namespace pal4
