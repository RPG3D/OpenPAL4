#include "pal4/game_data_property_capacity.h"

namespace pal4 {
namespace {

constexpr std::uint32_t kRecordBytes = 168;

void DestroyRecord(std::uint32_t record,
                   IGameDataPropertyRecordArrayRuntime& runtime) {
    for (std::uint32_t i = 7; i > 0; --i)
        runtime.DestroyTextRecord(record + 16 + (i - 1) * 16);
}

}  // namespace

std::uint32_t GameDataPropertyCapacity::Reserve(
    std::uint32_t table, std::int32_t requested,
    IGameDataPropertyCapacityRuntime& runtime) {
    constexpr std::uint32_t kIndexBytes = 8;
    const auto old_records = runtime.ReadWord(table + 96);
    const auto record_capacity = old_records
                                     ? (runtime.ReadWord(table + 104) -
                                        old_records) / kRecordBytes
                                     : 0;
    if (record_capacity < static_cast<std::uint32_t>(requested)) {
        const auto new_records = runtime.Allocate(
            kRecordBytes * static_cast<std::uint32_t>(requested < 0 ? 0 : requested));
        const auto old_end = runtime.ReadWord(table + 100);
        for (auto src = old_records, dst = new_records;
             src != old_end; src += kRecordBytes, dst += kRecordBytes)
            runtime.CopyRecord(dst, src);
        for (auto record = old_records; record != old_end;
             record += kRecordBytes)
            DestroyRecord(record, runtime);
        runtime.Free(old_records);
        const auto count = old_records
                               ? (old_end - old_records) / kRecordBytes : 0;
        runtime.WriteWord(table + 104, new_records +
                                           kRecordBytes *
                                               static_cast<std::uint32_t>(requested));
        runtime.WriteWord(table + 96, new_records);
        runtime.WriteWord(table + 100, new_records + count * kRecordBytes);
    }

    const auto old_index = runtime.ReadWord(table + 8);
    const auto index_capacity = old_index
                                    ? (runtime.ReadWord(table + 16) -
                                       old_index) / kIndexBytes
                                    : 0;
    if (index_capacity >= static_cast<std::uint32_t>(requested))
        return index_capacity;

    const auto new_index = runtime.Allocate(
        kIndexBytes * static_cast<std::uint32_t>(requested < 0 ? 0 : requested));
    const auto old_index_end = runtime.ReadWord(table + 12);
    for (auto src = old_index, dst = new_index;
         src != old_index_end; src += kIndexBytes, dst += kIndexBytes)
        runtime.CopyIndex(dst, src);
    runtime.Free(old_index);
    const auto count = old_index
                           ? (old_index_end - old_index) / kIndexBytes : 0;
    runtime.WriteWord(table + 16, new_index +
                                      kIndexBytes *
                                          static_cast<std::uint32_t>(requested));
    runtime.WriteWord(table + 8, new_index);
    const auto end = new_index + count * kIndexBytes;
    runtime.WriteWord(table + 12, end);
    return end;
}

std::uint32_t GameDataPropertyRecordArray::Insert(
    std::uint32_t array, std::uint32_t position,
    std::uint32_t source, IGameDataPropertyInsertRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    const auto offset = (position - begin) / kRecordBytes;
    (void)InsertCopies(array, position, 1, source, runtime);
    return runtime.ReadWord(array + 4) + offset * kRecordBytes;
}

std::uint32_t GameDataPropertyRecordArray::InsertCopies(
    std::uint32_t array, std::uint32_t position, std::uint32_t count,
    std::uint32_t source, IGameDataPropertyInsertRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    const auto end = runtime.ReadWord(array + 8);
    if (!count) return end;
    const auto capacity_end = runtime.ReadWord(array + 12);
    const auto tail_count = (end - position) / kRecordBytes;
    const auto growth_bytes = count * kRecordBytes;

    if ((capacity_end - end) / kRecordBytes >= count) {
        if (tail_count >= count) {
            for (auto current = end - growth_bytes; current != end;
                 current += kRecordBytes)
                runtime.CopyRecord(current + growth_bytes, current);
            for (auto current = end - growth_bytes;
                 current != position; current -= kRecordBytes)
                runtime.AssignRecord(current + growth_bytes - kRecordBytes,
                                     current - kRecordBytes);
            for (auto current = position;
                 current != position + growth_bytes;
                 current += kRecordBytes)
                runtime.AssignRecord(current, source);
        } else {
            for (auto current = position; current != end;
                 current += kRecordBytes)
                runtime.CopyRecord(current + growth_bytes, current);
            for (std::uint32_t i = tail_count; i < count; ++i)
                runtime.CopyRecord(end + (i - tail_count) * kRecordBytes,
                                   source);
            for (auto current = position; current != end;
                 current += kRecordBytes)
                runtime.AssignRecord(current, source);
        }
        const auto new_end = end + growth_bytes;
        runtime.WriteWord(array + 8, new_end);
        return new_end;
    }

    const auto old_count = begin ? (end - begin) / kRecordBytes : 0;
    const auto new_capacity = old_count +
                              (old_count > count ? old_count : count);
    const auto new_begin = runtime.Allocate(new_capacity * kRecordBytes);
    auto destination = new_begin;
    for (auto current = begin; current != position;
         current += kRecordBytes, destination += kRecordBytes)
        runtime.CopyRecord(destination, current);
    for (std::uint32_t i = 0; i < count; ++i) {
        runtime.CopyRecord(destination, source);
        destination += kRecordBytes;
    }
    for (auto current = position; current != end;
         current += kRecordBytes, destination += kRecordBytes)
        runtime.CopyRecord(destination, current);
    for (auto current = begin; current != end; current += kRecordBytes)
        DestroyRecord(current, runtime);
    runtime.Free(begin);
    runtime.WriteWord(array + 12,
                      new_begin + new_capacity * kRecordBytes);
    runtime.WriteWord(array + 4, new_begin);
    const auto new_end = new_begin + (old_count + count) * kRecordBytes;
    runtime.WriteWord(array + 8, new_end);
    return new_end;
}

std::uint32_t GameDataPropertyRecordArray::Size(
    std::uint32_t array, IGameDataPropertyRecordArrayRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    return begin ? (runtime.ReadWord(array + 8) - begin) / kRecordBytes : 0;
}

std::uint32_t GameDataPropertyRecordArray::CopyConstructRange(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination,
    IGameDataPropertyRecordArrayRuntime& runtime) {
    for (auto source = begin; source != end;
         source += kRecordBytes, destination += kRecordBytes)
        runtime.CopyRecord(destination, source);
    return destination;
}

void GameDataPropertyRecordArray::CopyConstructCopies(
    std::uint32_t destination, std::uint32_t count,
    std::uint32_t source,
    IGameDataPropertyRecordArrayRuntime& runtime) {
    for (std::uint32_t i = 0; i < count; ++i)
        runtime.CopyRecord(destination + i * kRecordBytes, source);
}

}  // namespace pal4
