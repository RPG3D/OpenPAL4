#include "pal4/game_data_ai_record_sorter.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace pal4 {
namespace {

constexpr std::uint32_t kKeyBytes = 8;

bool KeyLess(const GameDataAiKeyEntry& left,
             const GameDataAiKeyEntry& right) {
    return static_cast<std::int32_t>(left.key) <
           static_cast<std::int32_t>(right.key);
}

bool SignedLess(std::uint32_t left, std::uint32_t right) {
    return static_cast<std::int32_t>(left) < static_cast<std::int32_t>(right);
}

template <typename Record>
std::size_t Compact(std::span<const Record> source,
                    std::span<Record> destination) {
    if (destination.size() < source.size())
        throw std::length_error("record destination is too short");
    if (source.empty()) return 0;
    destination[0] = source[0];
    std::size_t output = 1;
    auto previous = source[0][0];
    for (std::size_t index = 1; index < source.size(); ++index) {
        const auto current = source[index];
        if (previous == current[0]) continue;
        destination[output++] = current;
        previous = current[0];
    }
    return output;
}

template <typename Record, typename Runtime>
void SortRecords(std::uint32_t table, std::uint32_t record_bytes,
                 Runtime& runtime) {
    const auto records = runtime.ReadWord(table + 96);
    const auto old_end = runtime.ReadWord(table + 100);
    const auto old_count = records ? (old_end - records) / record_bytes : 0;
    std::vector<GameDataAiKeyEntry> keys;
    keys.reserve(old_count);

    for (std::uint32_t source = 0; source < old_count; ++source) {
        const Record record =
            runtime.ReadRecord(records + source * record_bytes);
        if (!keys.empty() && keys.back().key == record[0]) continue;
        const auto destination = static_cast<std::uint32_t>(keys.size());
        if (destination != source)
            runtime.WriteRecord(records + destination * record_bytes, record);
        keys.push_back({record[0], destination});
    }
    runtime.WriteWord(table + 100,
                      records + static_cast<std::uint32_t>(keys.size()) *
                                    record_bytes);

    runtime.WriteWord(table + 12, runtime.ReadWord(table + 8));
    for (const auto& entry : keys) runtime.AppendKey(table + 4, entry);

    GameDataAiKeyAlgorithms::Sort(keys);

    const auto key_begin = runtime.ReadWord(table + 8);
    for (std::uint32_t i = 0; i < keys.size(); ++i)
        runtime.WriteKey(key_begin + i * kKeyBytes, keys[i]);
}

}  // namespace

GameDataAiKeyEntry GameDataAiKeyAlgorithms::SelectMedian(
    GameDataAiKeyEntry first, GameDataAiKeyEntry middle,
    GameDataAiKeyEntry last) {
    if (SignedLess(first.key, middle.key)) {
        if (SignedLess(middle.key, last.key)) return middle;
        return SignedLess(first.key, last.key) ? last : first;
    }
    if (SignedLess(first.key, last.key)) return first;
    return SignedLess(middle.key, last.key) ? last : middle;
}

std::size_t GameDataAiKeyAlgorithms::Partition(
    std::vector<GameDataAiKeyEntry>& entries, std::size_t first,
    std::size_t last, std::uint32_t pivot) {
    if (first >= last || last > entries.size())
        throw std::out_of_range("invalid key partition range");
    std::size_t left = first;
    std::size_t right = last;
    while (true) {
        while (left < last && SignedLess(entries[left].key, pivot)) ++left;
        do { --right; } while (right > first &&
                                 SignedLess(pivot, entries[right].key));
        if (left == last) return last;
        if (right <= left) return left;
        std::swap(entries[left], entries[right]);
        ++left;
    }
}

std::size_t GameDataAiKeyAlgorithms::PartitionLarge(
    std::vector<GameDataAiKeyEntry>& entries, std::size_t first,
    std::size_t last) {
    if (first > last || last > entries.size())
        throw std::out_of_range("invalid key sort range");
    while (last - first > 16) {
        const auto pivot = SelectMedian(entries[first],
                                        entries[first + (last - first) / 2],
                                        entries[last - 1]);
        const auto middle = Partition(entries, first, last, pivot.key);
        if (last - middle > middle - first) {
            (void)PartitionLarge(entries, first, middle);
            first = middle;
        } else {
            (void)PartitionLarge(entries, middle, last);
            last = middle;
        }
    }
    return last - first;
}

std::uint32_t GameDataAiKeyAlgorithms::InsertSorted(
    std::vector<GameDataAiKeyEntry>& entries, std::size_t first,
    std::size_t position, GameDataAiKeyEntry entry) {
    if (first > position || position >= entries.size())
        throw std::out_of_range("invalid key insertion position");
    while (position > first && KeyLess(entry, entries[position - 1])) {
        entries[position] = entries[position - 1];
        --position;
    }
    entries[position] = entry;
    return entry.record_index;
}

void GameDataAiKeyAlgorithms::InsertionSort(
    std::vector<GameDataAiKeyEntry>& entries, std::size_t first,
    std::size_t last) {
    if (first > last || last > entries.size())
        throw std::out_of_range("invalid key insertion range");
    for (auto position = first + (first < last); position < last; ++position)
        (void)InsertSorted(entries, first, position, entries[position]);
}

void GameDataAiKeyAlgorithms::Sort(
    std::vector<GameDataAiKeyEntry>& entries) {
    (void)PartitionLarge(entries, 0, entries.size());
    InsertionSort(entries, 0, entries.size());
}

std::size_t GameDataAiKeyAlgorithms::LowerBound(
    std::span<const GameDataAiKeyEntry> entries, std::uint32_t key) {
    std::size_t first = 0;
    std::size_t count = entries.size();
    while (count) {
        const auto middle = count / 2;
        if (SignedLess(entries[first + middle].key, key)) {
            first += middle + 1;
            count -= middle + 1;
        } else {
            count = middle;
        }
    }
    return first;
}

std::size_t GameDataAiRecordCompactor::Compact56(
    std::span<const GameDataAiRecord> source,
    std::span<GameDataAiRecord> destination) {
    return Compact(source, destination);
}

std::size_t GameDataAiRecordCompactor::Compact68(
    std::span<const GameDataAiPropertyRecord> source,
    std::span<GameDataAiPropertyRecord> destination) {
    return Compact(source, destination);
}

void GameDataAiRecordSorter::Sort(
    std::uint32_t table, IGameDataAiRecordSortRuntime& runtime) {
    SortRecords<GameDataAiRecord>(table, 56, runtime);
}

void GameDataAiPropertySorter::Sort(
    std::uint32_t table, IGameDataAiPropertySortRuntime& runtime) {
    SortRecords<GameDataAiPropertyRecord>(table, 68, runtime);
}

}  // namespace pal4
