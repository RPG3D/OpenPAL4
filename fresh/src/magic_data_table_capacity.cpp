#include "pal4/magic_data_table.h"

namespace pal4 {

std::uint32_t MagicDataTable::Reserve(
    std::uint32_t table, std::int32_t requested,
    IMagicDataCapacityRuntime& runtime) {
    const auto requested_bits = static_cast<std::uint32_t>(requested);
    const auto record_begin = runtime.ReadWord(table + 96);
    const auto record_end = runtime.ReadWord(table + 100);
    const auto record_capacity_end = runtime.ReadWord(table + 104);
    const auto record_capacity = record_begin
        ? (record_capacity_end - record_begin) / kRecordBytes : 0;
    if (record_capacity < requested_bits) {
        const auto allocation_count = requested < 0 ? 0u : requested_bits;
        const auto new_begin = runtime.Allocate(
            allocation_count * kRecordBytes);
        auto destination = new_begin;
        for (auto source = record_begin; source != record_end;
             source += kRecordBytes, destination += kRecordBytes)
            runtime.CopyRecord(destination, source);
        for (auto source = record_begin; source != record_end;
             source += kRecordBytes)
            runtime.DestroyRecord(source);
        runtime.Free(record_begin);

        const auto active_count = record_begin
            ? (record_end - record_begin) / kRecordBytes : 0;
        runtime.WriteWord(table + 104,
                          new_begin + requested_bits * kRecordBytes);
        runtime.WriteWord(table + 96, new_begin);
        runtime.WriteWord(table + 100,
                          new_begin + active_count * kRecordBytes);
    }

    const auto index_begin = runtime.ReadWord(table + 8);
    const auto index_end = runtime.ReadWord(table + 12);
    const auto index_capacity_end = runtime.ReadWord(table + 16);
    const auto index_capacity = index_begin
        ? (index_capacity_end - index_begin) / 8 : 0;
    if (index_capacity >= requested_bits) return index_capacity;

    const auto allocation_count = requested < 0 ? 0u : requested_bits;
    const auto new_begin = runtime.Allocate(allocation_count * 8);
    auto destination = new_begin;
    for (auto source = index_begin; source != index_end;
         source += 8, destination += 8) {
        if (destination) runtime.CopyIndex(destination, source);
    }
    runtime.Free(index_begin);
    runtime.WriteWord(table + 16, new_begin + requested_bits * 8);
    runtime.WriteWord(table + 8, new_begin);
    const auto new_end = new_begin +
        (index_begin ? (index_end - index_begin) / 8 : 0) * 8;
    runtime.WriteWord(table + 12, new_end);
    return new_end;
}

}  // namespace pal4
