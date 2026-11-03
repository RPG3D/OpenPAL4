#include "pal4/dword_sort.h"

#include <utility>

namespace pal4 {

std::int32_t DwordSort::PartitionLarge(
    std::uint32_t* begin, std::uint32_t* end, Compare compare) {
    auto count = static_cast<std::int32_t>(end - begin);
    while (count > 16) {
        const auto first = *begin;
        const auto middle = begin[count / 2];
        const auto last = end[-1];
        std::uint32_t pivot;
        if (compare(first, middle)) {
            if (compare(middle, last)) {
                pivot = middle;
            } else {
                pivot = compare(first, last) ? last : first;
            }
        } else if (compare(first, last)) {
            pivot = first;
        } else {
            pivot = compare(middle, last) ? last : middle;
        }

        auto* left = begin;
        auto* right = end;
        while (true) {
            if (compare(*left, pivot)) {
                do {
                    ++left;
                } while (compare(*left, pivot));
            }
            do {
                --right;
            } while (compare(pivot, *right));
            if (right <= left) {
                break;
            }
            std::swap(*left, *right);
            ++left;
        }

        if (end - left > left - begin) {
            (void)PartitionLarge(begin, left, compare);
            begin = left;
        } else {
            (void)PartitionLarge(left, end, compare);
            end = left;
        }
        count = static_cast<std::int32_t>(end - begin);
    }
    return count;
}

std::uint32_t* DwordSort::InsertionSort(
    std::uint32_t* begin, std::uint32_t* end, Compare compare) {
    if (begin == end) {
        return end;
    }
    for (auto* current = begin + 1; current != end; ++current) {
        const auto value = *current;
        if (compare(value, *begin)) {
            for (auto* slot = current; slot != begin; --slot) {
                *slot = slot[-1];
            }
            *begin = value;
        } else {
            auto* slot = current;
            auto* previous = current - 1;
            if (compare(value, *previous)) {
                do {
                    *slot = *previous;
                    slot = previous;
                    --previous;
                } while (compare(value, *previous));
            }
            *slot = value;
        }
    }
    return end;
}

std::uintptr_t DwordSort::Sort(
    std::uint32_t* begin, std::uint32_t* end, Compare compare) {
    if (begin == end) {
        return 0;
    }
    if (begin + 1 == end) {
        return 1;
    }

    if (end - begin <= 16) {
        return reinterpret_cast<std::uintptr_t>(InsertionSort(begin, end, compare));
    }

    (void)PartitionLarge(begin, end, compare);
    (void)InsertionSort(begin, begin + 16, compare);

    // The first 16 values establish a minimum at begin. The remaining
    // insertions can use that value as the backward-scan sentinel.
    for (auto* current = begin + 16; current != end; ++current) {
        const auto value = *current;
        auto* slot = current;
        if (compare(value, slot[-1])) {
            do {
                *slot = slot[-1];
                --slot;
            } while (compare(value, slot[-1]));
        }
        *slot = value;
    }
    return reinterpret_cast<std::uintptr_t>(end);
}

}  // namespace pal4
