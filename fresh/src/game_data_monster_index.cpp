#include "pal4/game_data_monster_index.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace pal4 {
namespace {

constexpr std::uint32_t kRecordBytes = 596;
constexpr std::uint32_t kKeyBytes = 8;
constexpr std::uint32_t kKeyOffset = 400;

}  // namespace

void GameDataMonsterIndex::Normalize(
    std::uint32_t table, IGameDataMonsterIndexRuntime& runtime) {
    const auto begin = runtime.ReadWord(table + 96);
    const auto old_end = runtime.ReadWord(table + 100);
    auto duplicate = old_end;
    for (auto record = begin + kRecordBytes;
         begin != old_end && record != old_end;
         record += kRecordBytes) {
        if (runtime.ReadWord(record - kRecordBytes + kKeyOffset) ==
            runtime.ReadWord(record + kKeyOffset)) {
            duplicate = record - kRecordBytes;
            break;
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
        for (auto record = begin; record != new_end;
             record += kRecordBytes) {
            const auto index = (record - begin) / kRecordBytes;
            const auto key = runtime.ReadWord(record + kKeyOffset);
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
        const auto address = sorted_begin +
                             static_cast<std::uint32_t>(i) * kKeyBytes;
        runtime.WriteWord(address, entries[i].first);
        runtime.WriteWord(address + 4, entries[i].second);
    }
}

void GameDataMonsterIndex::DestroyRange(
    std::uint32_t begin, std::uint32_t end,
    IGameDataMonsterIndexRuntime& runtime) {
    for (auto record = begin; record != end; record += kRecordBytes)
        GameDataMonsterRecord::DestroyFullRecord(record, runtime);
}

std::uint32_t GameDataMonsterIndex::CompactFrom(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination, IGameDataMonsterIndexRuntime& runtime) {
    if (begin == end) return destination;
    runtime.CopyRecord(destination, begin);
    destination += kRecordBytes;
    auto previous = begin;
    for (auto source = begin + kRecordBytes; source != end;
         source += kRecordBytes) {
        if (runtime.ReadWord(previous + kKeyOffset) !=
            runtime.ReadWord(source + kKeyOffset)) {
            runtime.CopyRecord(destination, source);
            destination += kRecordBytes;
        }
        previous = source;
    }
    return destination;
}

bool GameDataMonsterIndex::EqualKeys(
    std::uint32_t left, std::uint32_t right,
    IGameDataMonsterIndexRuntime& runtime) {
    return runtime.ReadWord(left + kKeyOffset) ==
           runtime.ReadWord(right + kKeyOffset);
}

}  // namespace pal4
