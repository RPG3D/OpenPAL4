#include "pal4/game_data_parameter_index.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace pal4 {
namespace {

constexpr std::uint32_t kRecordBytes = 328;

std::uint32_t CompactFrom(std::uint32_t begin, std::uint32_t end,
                          std::uint32_t destination,
                          IGameDataParameterIndexRuntime& runtime) {
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

}  // namespace

void GameDataParameterIndex::Normalize(
    std::uint32_t table, IGameDataParameterIndexRuntime& runtime) {
    const auto begin = runtime.ReadWord(table + 96);
    const auto old_end = runtime.ReadWord(table + 100);
    auto duplicate = old_end;
    if (begin != old_end) {
        for (auto record = begin + kRecordBytes; record != old_end;
             record += kRecordBytes) {
            if (runtime.ReadWord(record - kRecordBytes) ==
                runtime.ReadWord(record)) {
                duplicate = record - kRecordBytes;
                break;
            }
        }
    }
    const auto new_end = duplicate == old_end
                             ? old_end
                             : CompactFrom(duplicate, old_end, duplicate,
                                           runtime);
    DestroyRange(new_end, old_end, runtime);
    runtime.WriteWord(table + 100, new_end);

    const auto key_begin = runtime.ReadWord(table + 8);
    runtime.WriteWord(table + 12, key_begin);
    if (begin != 0) {
        for (auto record = begin; record != new_end; record += kRecordBytes)
            runtime.InsertKey(table + 4, runtime.ReadWord(table + 12),
                              runtime.ReadWord(record),
                              (record - begin) / kRecordBytes);
    }

    const auto sorted_begin = runtime.ReadWord(table + 8);
    const auto sorted_end = runtime.ReadWord(table + 12);
    std::vector<std::pair<std::uint32_t, std::uint32_t>> entries;
    for (auto at = sorted_begin; at != sorted_end; at += 8)
        entries.emplace_back(runtime.ReadWord(at), runtime.ReadWord(at + 4));
    std::sort(entries.begin(), entries.end(), [](const auto& left,
                                                  const auto& right) {
        return static_cast<std::int32_t>(left.first) <
               static_cast<std::int32_t>(right.first);
    });
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto at = sorted_begin + static_cast<std::uint32_t>(i) * 8;
        runtime.WriteWord(at, entries[i].first);
        runtime.WriteWord(at + 4, entries[i].second);
    }
}

void GameDataParameterIndex::DestroyRange(
    std::uint32_t begin, std::uint32_t end,
    IGameDataParameterRecordDestroyRuntime& runtime) {
    for (auto record = begin; record != end; record += kRecordBytes)
        GameDataParameterRecordLifecycle::Destroy(record, runtime);
}

}  // namespace pal4
