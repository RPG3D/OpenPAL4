#include "pal4/singleton_collection_sort.h"

namespace pal4 {
namespace {

std::int32_t ElementCount(const std::uint32_t begin,
                          const std::uint32_t end) {
    return static_cast<std::int32_t>(end - begin) / 28;
}

void SwapRecords(const std::uint32_t left, const std::uint32_t right,
                 ISingletonCollectionSortRuntime& runtime) {
    const std::uint32_t temporary = runtime.AllocateTemporary();
    (void)SingletonCollectionMove::CopyConstruct(temporary, left, runtime);
    runtime.AssignText(left, right);
    constexpr std::uint32_t offsets[]{16u, 20u, 24u};
    for (const std::uint32_t offset : offsets) {
        runtime.WriteDword(left + offset,
                           runtime.ReadDword(right + offset));
    }
    (void)SingletonCollectionMove::CopyElement(right, temporary, runtime);
    (void)runtime.ReleaseText(temporary);
}

}  // namespace

void SingletonCollectionSort::SelectMedian(
    const std::uint32_t first, const std::uint32_t middle,
    const std::uint32_t last, const std::uint32_t destination,
    ISingletonCollectionSortRuntime& runtime) {
    std::uint32_t selected;
    if (runtime.Compare(first, middle)) {
        if (runtime.Compare(middle, last)) {
            selected = middle;
        } else {
            selected = runtime.Compare(first, last) ? last : first;
        }
    } else if (runtime.Compare(first, last)) {
        selected = first;
    } else {
        selected = runtime.Compare(middle, last) ? last : middle;
    }
    (void)SingletonCollectionMove::CopyConstruct(destination, selected,
                                                   runtime);
    (void)runtime.ReleaseText(first);
    (void)runtime.ReleaseText(middle);
    (void)runtime.ReleaseText(last);
}

void SingletonCollectionSort::InsertSortedElement(
    std::uint32_t destination, const std::uint32_t temporary,
    ISingletonCollectionSortRuntime& runtime) {
    std::uint32_t previous = destination - 28u;
    while (runtime.Compare(temporary, previous)) {
        (void)SingletonCollectionMove::CopyElement(destination, previous,
                                                   runtime);
        destination = previous;
        previous -= 28u;
    }
    runtime.AssignText(destination, temporary);
    constexpr std::uint32_t offsets[]{16u, 20u, 24u};
    for (const std::uint32_t offset : offsets) {
        runtime.WriteDword(destination + offset,
                           runtime.ReadDword(temporary + offset));
    }
    (void)runtime.ReleaseText(temporary);
}

void SingletonCollectionSort::InsertionSort(
    const std::uint32_t begin, const std::uint32_t end,
    ISingletonCollectionSortRuntime& runtime) {
    if (begin == end || begin + 28u == end) {
        return;
    }
    for (std::uint32_t current = begin + 28u;
         current != end; current += 28u) {
        const std::uint32_t temporary = runtime.AllocateTemporary();
        (void)SingletonCollectionMove::CopyConstruct(temporary, current,
                                                       runtime);
        if (runtime.Compare(temporary, begin)) {
            (void)SingletonCollectionMove::Backward(begin, current,
                                                     current + 28u, runtime);
            runtime.AssignText(begin, temporary);
            constexpr std::uint32_t offsets[]{16u, 20u, 24u};
            for (const std::uint32_t offset : offsets) {
                runtime.WriteDword(begin + offset,
                                   runtime.ReadDword(temporary + offset));
            }
        } else {
            const std::uint32_t copied = runtime.AllocateTemporary();
            (void)SingletonCollectionMove::CopyConstruct(copied, temporary,
                                                           runtime);
            InsertSortedElement(current, copied, runtime);
        }
        (void)runtime.ReleaseText(temporary);
    }
}

void SingletonCollectionSort::SwapCollectionElements(
    const std::uint32_t left, const std::uint32_t right,
    ISingletonCollectionSortRuntime& runtime) {
    const std::uint32_t temporary = runtime.AllocateTemporary();
    runtime.WriteByte(temporary, runtime.ReadByte(left));
    runtime.WriteDword(temporary + 4u, 0u);
    runtime.WriteDword(temporary + 8u, 0u);
    runtime.WriteDword(temporary + 12u, 0u);
    runtime.AssignText(temporary, left);

    constexpr std::uint32_t offsets[]{16u, 20u, 24u};
    for (const std::uint32_t offset : offsets) {
        runtime.WriteDword(temporary + offset,
                           runtime.ReadDword(left + offset));
    }
    runtime.AssignText(left, right);
    for (const std::uint32_t offset : offsets) {
        runtime.WriteDword(left + offset,
                           runtime.ReadDword(right + offset));
    }
    runtime.AssignText(right, temporary);
    for (const std::uint32_t offset : offsets) {
        runtime.WriteDword(right + offset,
                           runtime.ReadDword(temporary + offset));
    }
    (void)runtime.ReleaseText(temporary);
}

void SingletonCollectionSort::PartitionLarge(
    std::uint32_t begin, std::uint32_t end,
    ISingletonCollectionSortRuntime& runtime) {
    std::int32_t count = ElementCount(begin, end);
    while (count > 16) {
        const std::uint32_t last = runtime.AllocateTemporary();
        (void)SingletonCollectionMove::CopyConstruct(last, end - 28u,
                                                       runtime);
        const std::uint32_t middle = runtime.AllocateTemporary();
        const std::uint32_t middle_element =
            begin + static_cast<std::uint32_t>(count / 2) * 28u;
        (void)SingletonCollectionMove::CopyConstruct(middle, middle_element,
                                                       runtime);
        const std::uint32_t first = runtime.AllocateTemporary();
        (void)SingletonCollectionMove::CopyConstruct(first, begin, runtime);
        const std::uint32_t pivot = runtime.AllocateTemporary();
        SelectMedian(first, middle, last, pivot, runtime);

        std::uint32_t left = begin;
        std::uint32_t right = end;
        while (true) {
            while (runtime.Compare(left, pivot)) {
                left += 28u;
            }
            do {
                right -= 28u;
            } while (runtime.Compare(pivot, right));
            if (right <= left) {
                break;
            }
            SwapRecords(left, right, runtime);
            left += 28u;
        }
        (void)runtime.ReleaseText(pivot);

        const std::int32_t left_count = ElementCount(begin, left);
        const std::int32_t right_count = ElementCount(left, end);
        if (right_count <= left_count) {
            PartitionLarge(left, end, runtime);
            end = left;
        } else {
            PartitionLarge(begin, left, runtime);
            begin = left;
        }
        count = ElementCount(begin, end);
    }
}

}  // namespace pal4
