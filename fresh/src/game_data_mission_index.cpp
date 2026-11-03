#include "pal4/game_data_mission_index.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace pal4 {
namespace {

constexpr std::uint32_t kRecordBytes = 68;
constexpr std::uint32_t kKeyBytes = 8;

}  // namespace

void GameDataMissionIndex::Normalize(
    std::uint32_t table, IGameDataMissionIndexRuntime& runtime) {
    const auto begin = runtime.ReadWord(table + 96);
    const auto old_end = runtime.ReadWord(table + 100);
    auto duplicate = old_end;
    for (auto record = begin + kRecordBytes; begin != old_end &&
                                             record != old_end;
         record += kRecordBytes) {
        if (runtime.ReadWord(record - kRecordBytes) ==
            runtime.ReadWord(record)) {
            duplicate = record - kRecordBytes;
            break;
        }
    }

    auto new_end = old_end;
    if (duplicate != old_end)
        new_end = CompactFrom(duplicate, old_end, duplicate, runtime);
    (void)DestroyRange(new_end, old_end, runtime);
    runtime.WriteWord(table + 100, new_end);

    const auto key_begin = runtime.ReadWord(table + 8);
    runtime.WriteWord(table + 12, key_begin);
    if (begin != 0) {
        for (auto record = begin; record != new_end;
             record += kRecordBytes) {
            const auto index = (record - begin) / kRecordBytes;
            const auto key = runtime.ReadWord(record);
            runtime.InsertKey(table + 4, runtime.ReadWord(table + 12),
                              key, index);
        }
    }

    const auto sorted_begin = runtime.ReadWord(table + 8);
    const auto sorted_end = runtime.ReadWord(table + 12);
    std::vector<std::pair<std::uint32_t, std::uint32_t>> entries;
    for (auto at = sorted_begin; at != sorted_end; at += kKeyBytes)
        entries.emplace_back(runtime.ReadWord(at), runtime.ReadWord(at + 4));
    std::sort(entries.begin(), entries.end(), [](const auto& left,
                                                  const auto& right) {
        return static_cast<std::int32_t>(left.first) <
               static_cast<std::int32_t>(right.first);
    });
    for (std::size_t i = 0; i < entries.size(); ++i) {
        runtime.WriteWord(sorted_begin + static_cast<std::uint32_t>(i) *
                                             kKeyBytes,
                          entries[i].first);
        runtime.WriteWord(sorted_begin + static_cast<std::uint32_t>(i) *
                                             kKeyBytes + 4,
                          entries[i].second);
    }
}

std::uint32_t GameDataMissionIndex::DestroyRange(
    std::uint32_t begin, std::uint32_t end,
    IGameDataMissionIndexRuntime& runtime) {
    std::uint32_t result = 0;
    for (auto record = begin; record != end; record += kRecordBytes)
        result = runtime.DestroyRecord(record);
    return result;
}

std::uint32_t GameDataMissionIndex::CompactFrom(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination,
    IGameDataMissionIndexRuntime& runtime) {
    if (begin == end) return destination;
    runtime.CopyRecord(destination, begin);
    destination += kRecordBytes;
    auto previous = begin;
    for (auto source = begin + kRecordBytes; source != end;
         source += kRecordBytes) {
        if (runtime.ReadWord(previous) != runtime.ReadWord(source)) {
            runtime.CopyRecord(destination, source);
            destination += kRecordBytes;
        }
        previous = source;
    }
    return destination;
}

}  // namespace pal4
