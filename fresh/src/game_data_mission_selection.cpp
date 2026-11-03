#include "pal4/game_data_mission_selection.h"

#include <algorithm>
#include <new>
#include <stdexcept>

namespace pal4 {
namespace {

using Records = std::vector<std::uint32_t>;

bool Before(std::uint32_t left, std::uint32_t right,
            IGameDataMissionSelectionRuntime& runtime) {
    return static_cast<std::int32_t>(runtime.ReadWord(left + 8)) >
           static_cast<std::int32_t>(runtime.ReadWord(right + 8));
}

std::uint32_t Median(std::uint32_t first, std::uint32_t middle,
                     std::uint32_t last,
                     IGameDataMissionSelectionRuntime& runtime) {
    if (Before(first, middle, runtime)) {
        if (Before(middle, last, runtime)) return middle;
        return Before(first, last, runtime) ? last : first;
    }
    if (Before(first, last, runtime)) return first;
    return Before(middle, last, runtime) ? last : middle;
}

std::size_t Partition(Records& records, std::size_t first,
                      std::size_t last, std::uint32_t pivot,
                      IGameDataMissionSelectionRuntime& runtime) {
    auto left = first;
    auto right = last;
    while (true) {
        while (left < last && Before(records[left], pivot, runtime)) ++left;
        do {
            --right;
        } while (right > first && Before(pivot, records[right], runtime));
        if (left == last) return last;
        if (right <= left) return left;
        std::swap(records[left], records[right]);
        ++left;
    }
}

void PartitionLarge(Records& records, std::size_t first,
                    std::size_t last,
                    IGameDataMissionSelectionRuntime& runtime) {
    while (last - first > 16) {
        const auto pivot = Median(records[first],
                                  records[first + (last - first) / 2],
                                  records[last - 1], runtime);
        const auto middle = Partition(records, first, last, pivot, runtime);
        if (last - middle > middle - first) {
            PartitionLarge(records, first, middle, runtime);
            first = middle;
        } else {
            PartitionLarge(records, middle, last, runtime);
            last = middle;
        }
    }
}

void InsertionSort(Records& records,
                   IGameDataMissionSelectionRuntime& runtime) {
    for (std::size_t position = 1; position < records.size(); ++position) {
        const auto entry = records[position];
        auto current = position;
        while (current > 0 && Before(entry, records[current - 1], runtime)) {
            records[current] = records[current - 1];
            --current;
        }
        records[current] = entry;
    }
}

void Append(GameDataMissionPointerArray& output, std::uint32_t record,
            IGameDataMissionSelectionRuntime& runtime) {
    if (output.records.size() == output.capacity) {
        const auto next_capacity = output.capacity ? output.capacity * 2 : 1;
        const auto next = runtime.AllocatePointerStorage(next_capacity);
        if (!next) throw std::bad_alloc();
        if (output.begin_address)
            runtime.FreePointerStorage(output.begin_address);
        output.begin_address = next;
        output.capacity = next_capacity;
    }
    output.records.push_back(record);
}

}  // namespace

std::uint32_t GameDataMissionSelection::CollectAndSort(
    std::uint32_t manager, GameDataMissionPointerArray& output,
    IGameDataMissionSelectionRuntime& runtime) {
    const auto table = runtime.ReadWord(manager + 4);
    output.records.clear();
    const auto begin = runtime.ReadWord(table + 96);
    if (begin) {
        const auto end = runtime.ReadWord(table + 100);
        const auto count = static_cast<std::int32_t>(end - begin) / 68;
        for (std::int32_t index = 0; index < count; ++index)
            Append(output, begin + static_cast<std::uint32_t>(index) * 68,
                   runtime);
    }
    if (output.records.empty()) return 0;
    PartitionLarge(output.records, 0, output.records.size(), runtime);
    InsertionSort(output.records, runtime);
    return output.begin_address +
           static_cast<std::uint32_t>(output.records.size() * 4);
}

}  // namespace pal4
