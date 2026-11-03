#include "pal4/skill_event_sorter.h"

#include <stdexcept>
#include <utility>

namespace pal4 {
namespace {

bool Less(SkillEventRegistration left, SkillEventRegistration right) {
    const auto left_id = static_cast<std::int32_t>(left.id);
    const auto right_id = static_cast<std::int32_t>(right.id);
    return left_id < right_id ||
           (left_id == right_id && left.callback < right.callback);
}

}  // namespace

bool SkillEventSorter::CompareValues(SkillEventRegistration first,
                                     SkillEventRegistration second) {
    return Less(first, second);
}

SkillEventRegistration SkillEventSorter::SelectMedian(
    SkillEventRegistration first, SkillEventRegistration middle,
    SkillEventRegistration last) {
    if (Less(middle, first)) {
        if (Less(first, last)) return first;
        return Less(middle, last) ? last : middle;
    }
    if (Less(middle, last)) return middle;
    return Less(first, last) ? last : first;
}

std::size_t SkillEventSorter::Partition(
    std::span<SkillEventRegistration> entries,
    SkillEventRegistration pivot) {
    if (entries.empty()) throw std::invalid_argument("empty skill event partition");
    std::size_t left = 0u;
    std::size_t right = entries.size();
    while (true) {
        while (left < right && Less(entries[left], pivot)) ++left;
        do { --right; } while (right > left && Less(pivot, entries[right]));
        if (right <= left) return left;
        std::swap(entries[left], entries[right]);
        ++left;
    }
}

void SkillEventSorter::InsertionSort(
    std::span<SkillEventRegistration> entries) {
    for (std::size_t i = 1u; i < entries.size(); ++i) {
        const auto value = entries[i];
        auto cursor = i;
        while (cursor > 0u && Less(value, entries[cursor - 1u])) {
            entries[cursor] = entries[cursor - 1u];
            --cursor;
        }
        entries[cursor] = value;
    }
}

std::size_t SkillEventSorter::SortLargePartitions(
    std::span<SkillEventRegistration> entries) {
    while (entries.size() > 16u) {
        const auto pivot = SelectMedian(entries.front(),
                                        entries[entries.size() / 2u],
                                        entries.back());
        const auto split = Partition(entries, pivot);
        if (split == 0u || split == entries.size())
            throw std::logic_error("skill event partition did not shrink");
        auto left = entries.first(split);
        auto right = entries.subspan(split);
        if (right.size() > left.size()) {
            static_cast<void>(SortLargePartitions(left));
            entries = right;
        } else {
            static_cast<void>(SortLargePartitions(right));
            entries = left;
        }
    }
    return entries.size();
}

void SkillEventSorter::Sort(std::span<SkillEventRegistration> entries) {
    if (entries.size() > 16u)
        static_cast<void>(SortLargePartitions(entries));
    InsertionSort(entries);
}

}  // namespace pal4
