#include "pal4/skill_data_table.h"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace pal4 {
namespace {

struct SkillIndexEntry {
    std::uint32_t key;
    std::uint32_t position;
};

}  // namespace

void SkillDataTable::RebuildIndex(
    std::uint32_t table, ISkillDataIndexRuntime& runtime) {
    const auto begin = runtime.ReadWord(table + 96);
    const auto old_end = runtime.ReadWord(table + 100);
    auto duplicate = old_end;
    for (auto record = begin + kSkillRecordBytes;
         begin != old_end && record != old_end;
         record += kSkillRecordBytes) {
        if (runtime.ReadWord(record - kSkillRecordBytes) ==
            runtime.ReadWord(record)) {
            duplicate = record - kSkillRecordBytes;
            break;
        }
    }

    auto new_end = old_end;
    if (duplicate != old_end) {
        auto destination = duplicate;
        runtime.AssignRecord(destination, duplicate);
        destination += kSkillRecordBytes;
        auto previous = duplicate;
        for (auto source = duplicate + kSkillRecordBytes;
             source != old_end; source += kSkillRecordBytes) {
            if (runtime.ReadWord(previous) != runtime.ReadWord(source)) {
                runtime.AssignRecord(destination, source);
                destination += kSkillRecordBytes;
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
             record += kSkillRecordBytes) {
            const auto index = (record - begin) / kSkillRecordBytes;
            runtime.InsertKey(table + 4, runtime.ReadWord(table + 12),
                              runtime.ReadWord(record), index);
        }
    }

    const auto sorted_begin = runtime.ReadWord(table + 8);
    const auto sorted_end = runtime.ReadWord(table + 12);
    std::vector<SkillIndexEntry> entries;
    for (auto address = sorted_begin; address != sorted_end;
         address += 8)
        entries.push_back({runtime.ReadWord(address),
                           runtime.ReadWord(address + 4)});
    std::sort(entries.begin(), entries.end(),
              [](const SkillIndexEntry& left,
                 const SkillIndexEntry& right) {
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

std::uint32_t SkillDataTable::DestroyRange(
    std::uint32_t begin, std::uint32_t end,
    ISkillDataIndexRuntime& runtime) {
    std::uint32_t result = 0;
    for (auto record = begin; record != end;
         record += kSkillRecordBytes)
        result = runtime.DestroyRecord(record);
    return result;
}

}  // namespace pal4
