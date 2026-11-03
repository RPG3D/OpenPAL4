#include "pal4/game_data_role_table_capacity.h"

namespace pal4 {

std::uint32_t GameDataRoleTableCapacity::Reserve(
    std::uint32_t table, std::int32_t requested,
    IGameDataRoleTableCapacityRuntime& runtime) {
    constexpr std::uint32_t kRecordBytes = 320;
    constexpr std::uint32_t kIndexBytes = 8;
    const auto requested_unsigned = static_cast<std::uint32_t>(requested);
    const auto allocation_count = static_cast<std::uint32_t>(
        requested < 0 ? 0 : requested);

    const auto old_records = runtime.ReadWord(table + 96);
    const auto record_capacity = old_records
                                     ? (runtime.ReadWord(table + 104) -
                                        old_records) / kRecordBytes
                                     : 0;
    if (record_capacity < requested_unsigned) {
        const auto new_records = runtime.Allocate(
            kRecordBytes * allocation_count);
        const auto old_end = runtime.ReadWord(table + 100);
        for (auto source = old_records, destination = new_records;
             source != old_end;
             source += kRecordBytes, destination += kRecordBytes)
            runtime.CopyRecord(destination, source);
        for (auto record = old_records; record != old_end;
             record += kRecordBytes)
            runtime.DestroyRecord(record);
        runtime.Free(old_records);
        const auto count = old_records
                               ? (old_end - old_records) / kRecordBytes : 0;
        runtime.WriteWord(table + 104,
                          new_records + kRecordBytes * requested_unsigned);
        runtime.WriteWord(table + 96, new_records);
        runtime.WriteWord(table + 100,
                          new_records + count * kRecordBytes);
    }

    const auto old_index = runtime.ReadWord(table + 8);
    const auto index_capacity = old_index
                                    ? (runtime.ReadWord(table + 16) -
                                       old_index) / kIndexBytes
                                    : 0;
    if (index_capacity >= requested_unsigned) return index_capacity;

    const auto new_index = runtime.Allocate(kIndexBytes * allocation_count);
    const auto old_index_end = runtime.ReadWord(table + 12);
    for (auto source = old_index, destination = new_index;
         source != old_index_end;
         source += kIndexBytes, destination += kIndexBytes)
        if (destination) runtime.CopyIndex(destination, source);
    runtime.Free(old_index);
    const auto count = old_index
                           ? (old_index_end - old_index) / kIndexBytes : 0;
    runtime.WriteWord(table + 16,
                      new_index + kIndexBytes * requested_unsigned);
    runtime.WriteWord(table + 8, new_index);
    const auto end = new_index + count * kIndexBytes;
    runtime.WriteWord(table + 12, end);
    return end;
}

}  // namespace pal4
