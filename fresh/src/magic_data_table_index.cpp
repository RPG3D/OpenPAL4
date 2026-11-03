#include "pal4/magic_data_table.h"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace pal4 {
namespace {

struct MagicIndexEntry {
    std::uint32_t key;
    std::uint32_t position;
};

}  // namespace

void MagicDataTable::RebuildIndex(
    std::uint32_t table, IMagicDataIndexRuntime& runtime) {
    const auto begin = runtime.ReadWord(table + 96);
    const auto old_end = runtime.ReadWord(table + 100);
    auto duplicate = old_end;
    for (auto record = begin + kRecordBytes;
         begin != old_end && record != old_end;
         record += kRecordBytes) {
        if (runtime.ReadWord(record - kRecordBytes) ==
            runtime.ReadWord(record)) {
            duplicate = record - kRecordBytes;
            break;
        }
    }

    auto new_end = old_end;
    if (duplicate != old_end) {
        auto destination = duplicate;
        runtime.AssignRecord(destination, duplicate);
        destination += kRecordBytes;
        auto previous = duplicate;
        for (auto source = duplicate + kRecordBytes;
             source != old_end; source += kRecordBytes) {
            if (runtime.ReadWord(previous) != runtime.ReadWord(source)) {
                runtime.AssignRecord(destination, source);
                destination += kRecordBytes;
            }
            previous = source;
        }
        new_end = destination;
    }
    static_cast<void>(DestroyRange(new_end, old_end, runtime));
    runtime.WriteWord(table + 100, new_end);

    const auto index_begin = runtime.ReadWord(table + 8);
    runtime.WriteWord(table + 12, index_begin);
    if (begin != 0) {
        for (auto record = begin; record != new_end;
             record += kRecordBytes) {
            const auto index = (record - begin) / kRecordBytes;
            runtime.InsertKey(table + 4, runtime.ReadWord(table + 12),
                              runtime.ReadWord(record), index);
        }
    }

    const auto sorted_begin = runtime.ReadWord(table + 8);
    const auto sorted_end = runtime.ReadWord(table + 12);
    std::vector<MagicIndexEntry> entries;
    for (auto address = sorted_begin; address != sorted_end;
         address += 8)
        entries.push_back({runtime.ReadWord(address),
                           runtime.ReadWord(address + 4)});
    std::sort(entries.begin(), entries.end(),
              [](const MagicIndexEntry& left,
                 const MagicIndexEntry& right) {
                  return static_cast<std::int32_t>(left.key) <
                         static_cast<std::int32_t>(right.key);
              });
    for (std::size_t index = 0; index < entries.size(); ++index) {
        const auto address = sorted_begin +
            static_cast<std::uint32_t>(index) * 8;
        runtime.WriteWord(address, entries[index].key);
        runtime.WriteWord(address + 4, entries[index].position);
    }
}

std::uint32_t MagicDataTable::DestroyRange(
    std::uint32_t begin, std::uint32_t end,
    IMagicDataIndexRuntime& runtime) {
    std::uint32_t result = 0;
    for (auto record = begin; record != end;
         record += kRecordBytes)
        result = runtime.DestroyRecord(record);
    return result;
}

}  // namespace pal4
