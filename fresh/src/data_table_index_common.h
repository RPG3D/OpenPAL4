#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

namespace pal4::detail {

struct DataTableIndexEntry {
    std::uint32_t key;
    std::uint32_t position;
};

template <std::uint32_t Stride, class Runtime>
std::uint32_t DestroyDataTableRange(
    std::uint32_t begin, std::uint32_t end, Runtime& runtime) {
    std::uint32_t result = 0;
    for (auto record = begin; record != end; record += Stride)
        result = runtime.DestroyRecord(record);
    return result;
}

template <std::uint32_t Stride, class Runtime>
void RebuildDataTableIndex(std::uint32_t table, Runtime& runtime) {
    const auto begin = runtime.ReadWord(table + 96);
    const auto old_end = runtime.ReadWord(table + 100);
    auto duplicate = old_end;
    if (begin != old_end) {
        for (auto record = begin + Stride; record != old_end;
             record += Stride) {
            if (runtime.ReadWord(record - Stride) ==
                runtime.ReadWord(record)) {
                duplicate = record - Stride;
                break;
            }
        }
    }

    const auto new_end = runtime.CopyUnique(duplicate, old_end, duplicate);
    static_cast<void>(DestroyDataTableRange<Stride>(new_end, old_end,
                                                     runtime));
    runtime.WriteWord(table + 100, new_end);

    const auto index_begin = runtime.ReadWord(table + 8);
    runtime.WriteWord(table + 12, index_begin);
    if (begin) {
        std::uint32_t index = 0;
        for (auto record = begin; record != new_end;
             record += Stride, ++index)
            runtime.InsertKey(table + 4, runtime.ReadWord(table + 12),
                              runtime.ReadWord(record), index);
    }

    const auto sorted_begin = runtime.ReadWord(table + 8);
    const auto sorted_end = runtime.ReadWord(table + 12);
    std::vector<DataTableIndexEntry> entries;
    for (auto address = sorted_begin; address != sorted_end; address += 8)
        entries.push_back({runtime.ReadWord(address),
                           runtime.ReadWord(address + 4)});
    std::sort(entries.begin(), entries.end(),
              [](const DataTableIndexEntry& left,
                 const DataTableIndexEntry& right) {
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

}  // namespace pal4::detail
