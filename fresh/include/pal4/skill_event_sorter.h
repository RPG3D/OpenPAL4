#pragma once

#include "pal4/skill_system_event_registry.h"

#include <cstddef>
#include <span>

namespace pal4 {

class SkillEventSorter final {
public:
    // IDA 0x58abf0: the second pair is greater than the first pair.
    [[nodiscard]] static bool CompareValues(SkillEventRegistration first,
                                            SkillEventRegistration second);
    // IDA 0x58aae0.
    [[nodiscard]] static SkillEventRegistration SelectMedian(
        SkillEventRegistration first, SkillEventRegistration middle,
        SkillEventRegistration last);
    // IDA 0x58ab90: returns the first index of the right partition.
    [[nodiscard]] static std::size_t Partition(
        std::span<SkillEventRegistration> entries,
        SkillEventRegistration pivot);
    // IDA 0x58aa50.
    static void InsertionSort(std::span<SkillEventRegistration> entries);
    // IDA 0x58a900: partitions recursively until every remainder has <=16 items.
    [[nodiscard]] static std::size_t SortLargePartitions(
        std::span<SkillEventRegistration> entries);
    // IDA 0x58a170: native partition phase followed by insertion cleanup.
    static void Sort(std::span<SkillEventRegistration> entries);
};

}  // namespace pal4
