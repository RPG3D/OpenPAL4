#include "pal4/collection_pointer_group.h"

#include <algorithm>
#include <cstring>
#include <new>

namespace pal4 {

std::uint32_t CollectionPointerSortRuntimeStub::ReadWord(std::uint32_t) {
    return 0u;
}
void CollectionPointerSortRuntimeStub::WriteWord(
    std::uint32_t, std::uint32_t) {}
std::uint32_t CollectionPointerSortRuntimeStub::AllocateMemory(
    std::uint32_t) { return 0u; }
void CollectionPointerSortRuntimeStub::FreeMemory(std::uint32_t) {}
void CollectionPointerSortRuntimeStub::InsertionSort(
    std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t) {}
std::uint32_t CollectionPointerSortRuntimeStub::CompareThree(
    std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t) { return 0u; }
std::uint32_t CollectionPointerSortRuntimeStub::Partition(
    std::uint32_t begin, std::uint32_t end, std::uint32_t) {
    return begin + ((end - begin) / 8u) * 4u;
}
void CollectionPointerSortRuntimeStub::PartitionRange(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
std::int32_t CollectionPointerSortRuntimeStub::CompareItems(
    std::uint32_t, std::uint32_t) { return 0; }
std::uint32_t CollectionNameLookupRuntimeStub::ReadWord(std::uint32_t) {
    return 0u;
}
std::uint8_t CollectionNameLookupRuntimeStub::ReadByte(std::uint32_t) {
    return 0u;
}
std::uint32_t CollectionNameLookupRuntimeStub::GetNameRecord(
    std::uint32_t, std::uint32_t) { return 0u; }

std::uint32_t CollectionPointerGroupRuntimeStub::ReadWord(std::uint32_t) {
    return 0u;
}
void CollectionPointerGroupRuntimeStub::WriteWord(
    std::uint32_t, std::uint32_t) {}
std::uint8_t CollectionPointerGroupRuntimeStub::ReadByte(std::uint32_t) {
    return 0u;
}
void CollectionPointerGroupRuntimeStub::WriteByte(
    std::uint32_t, std::uint8_t) {}
std::uint32_t CollectionPointerGroupRuntimeStub::FreeMemory(
    std::uint32_t address) { return address; }
void CollectionPointerGroupRuntimeStub::DeleteObject(
    std::uint32_t, std::uint32_t) {}

std::uint32_t CollectionPointerGroup::CleanupStringHolder(
    const std::uint32_t holder, ICollectionPointerGroupRuntime& runtime) {
    const auto data = runtime.ReadWord(holder + 12u);
    auto result = data;
    if (data != 0u) {
        const auto header = data - 1u;
        const auto references = runtime.ReadByte(header);
        result = header;
        if (references == 0u || references == 0xffu)
            result = runtime.FreeMemory(header);
        else
            runtime.WriteByte(header,
                static_cast<std::uint8_t>(references - 1u));
    }
    runtime.WriteWord(holder + 12u, 0u);
    runtime.WriteWord(holder + 16u, 0u);
    runtime.WriteWord(holder + 20u, 0u);
    return result;
}

std::uint32_t CollectionPointerGroup::Clear(
    CollectionPointerGroupState& group,
    ICollectionPointerGroupRuntime& runtime) {
    for (auto slot = group.first_begin; slot != group.first_end;
         slot += 4u) {
        const auto object = runtime.ReadWord(slot);
        if (object != 0u) runtime.DeleteObject(object, 1u);
    }
    group.first_end = group.first_begin;
    group.second_end = group.second_begin;
    const auto result = group.third_end;
    for (auto slot = group.third_begin; slot != group.third_end;
         slot += 4u) {
        const auto holder = runtime.ReadWord(slot);
        if (holder == 0u) continue;
        (void)CleanupStringHolder(holder, runtime);
        (void)runtime.FreeMemory(holder);
    }
    group.third_end = group.third_begin;
    return result;
}

std::uint32_t CollectionPointerGroup::Cleanup(
    CollectionPointerGroupState& group,
    ICollectionPointerGroupRuntime& runtime) {
    (void)Clear(group, runtime);
    (void)runtime.FreeMemory(group.third_begin);
    group.third_begin = group.third_end = group.third_capacity = 0u;
    (void)runtime.FreeMemory(group.second_begin);
    group.second_begin = group.second_end = group.second_capacity = 0u;
    const auto result = runtime.FreeMemory(group.first_begin);
    group.first_begin = group.first_end = group.first_capacity = 0u;
    return result;
}

std::int32_t CollectionPointerGroup::ProcessAll(
    CollectionPointerGroupState& group, const std::uint32_t source,
    const std::uint32_t input, ICollectionProcessAllRuntime& runtime) {
    CollectionPointerVisitorState visitor{0x849718u, input, 0u};
    const auto iterator = runtime.CreateIterator(source);
    std::uint32_t end_result = 0u;
    try {
        while (((end_result = runtime.IsEnd(iterator, 24u)) & 0xffu) == 0u) {
            const auto element = runtime.Current(iterator, 12u);
            visitor.output = 0u;
            runtime.ProcessCurrent(element, visitor, 12u);
            DwordCollectionSpanState array{
                group.vtable, group.first_begin, group.first_end,
                group.first_capacity};
            (void)CollectionPointerArray::PushBack(
                array, visitor.output, runtime);
            group.first_begin = array.begin;
            group.first_end = array.end;
            group.first_capacity = array.capacity;
            runtime.Advance(iterator, 36u);
        }
    } catch (...) {
        if (iterator != 0u)
            (void)runtime.DestroyIterator(iterator, 1u);
        throw;
    }
    return iterator == 0u ? static_cast<std::int32_t>(end_result) :
        runtime.DestroyIterator(iterator, 1u);
}

void CollectionPointerGroup::Sort(
    CollectionPointerGroupState& group,
    ICollectionPointerSortRuntime& runtime) {
    const auto source_count = group.first_begin == 0u ? 0u :
        (group.first_end - group.first_begin) / 4u;
    const auto destination_count = group.second_begin == 0u ? 0u :
        (group.second_end - group.second_begin) / 4u;
    if (source_count <= destination_count) {
        for (std::uint32_t index = 0u; index < source_count; ++index) {
            runtime.WriteWord(group.second_begin + index * 4u,
                              runtime.ReadWord(group.first_begin + index * 4u));
        }
        group.second_end = group.second_begin + source_count * 4u;
    } else {
        const auto destination_capacity = group.second_begin == 0u ? 0u :
            (group.second_capacity - group.second_begin) / 4u;
        if (source_count <= destination_capacity) {
            for (std::uint32_t index = 0u; index < source_count; ++index) {
                const auto target = index < destination_count ?
                    group.second_begin + index * 4u :
                    group.second_end + (index - destination_count) * 4u;
                runtime.WriteWord(target,
                    runtime.ReadWord(group.first_begin + index * 4u));
            }
            group.second_end = group.second_begin + source_count * 4u;
        } else {
            runtime.FreeMemory(group.second_begin);
            const auto memory = runtime.AllocateMemory(source_count * 4u);
            if (memory == 0u && source_count != 0u) {
                group.second_begin = 0u;
                group.second_end = 0u;
                group.second_capacity = 0u;
                throw std::bad_alloc{};
            }
            group.second_begin = memory;
            for (std::uint32_t index = 0u; index < source_count; ++index) {
                runtime.WriteWord(memory + index * 4u,
                    runtime.ReadWord(group.first_begin + index * 4u));
            }
            group.second_end = memory + source_count * 4u;
            group.second_capacity = group.second_end;
        }
    }

    const auto begin = group.second_begin;
    const auto end = group.second_end;
    if ((end - begin) / 4u <= 16u) {
        runtime.InsertionSort(begin, end, 0u, end);
        return;
    }

    auto left = begin;
    auto right = end;
    do {
        const auto midpoint = left + ((right - left) / 8u) * 4u;
        const auto pivot = runtime.CompareThree(
            runtime.ReadWord(left), runtime.ReadWord(midpoint),
            runtime.ReadWord(right - 4u), end);
        const auto split = runtime.Partition(left, right, pivot);
        if (right - split > split - left) {
            runtime.PartitionRange(left, split, end);
            left = split;
        } else {
            runtime.PartitionRange(split, right, end);
            right = split;
        }
    } while (right - left > 64u);

    runtime.InsertionSort(begin, begin + 64u, 0u, end);
    for (auto current = begin + 64u; current != end; current += 4u) {
        const auto candidate = runtime.ReadWord(current);
        auto position = current;
        while (position > begin && runtime.CompareItems(
                   candidate, runtime.ReadWord(position - 4u)) < 0) {
            runtime.WriteWord(position,
                              runtime.ReadWord(position - 4u));
            position -= 4u;
        }
        runtime.WriteWord(position, candidate);
    }
}

std::int32_t CollectionPointerGroup::CompareName(
    const std::uint32_t record, const char* const name,
    ICollectionNameLookupRuntime& runtime) {
    const auto length = runtime.ReadWord(record + 8u);
    const auto name_length = std::strlen(name);
    const auto compared = std::min<std::uint32_t>(
        length, static_cast<std::uint32_t>(name_length));
    const auto data = runtime.ReadWord(record + 4u);
    for (std::uint32_t index = 0u; index < compared; ++index) {
        const auto left = runtime.ReadByte(data + index);
        const auto right = static_cast<std::uint8_t>(name[index]);
        if (left != right) return left < right ? -1 : 1;
    }
    if (length == name_length) return 0;
    return length < name_length ? -1 : 1;
}

std::uint32_t CollectionPointerGroup::FindByName(
    const CollectionPointerGroupState& group, const char* const name,
    ICollectionNameLookupRuntime& runtime) {
    auto position = group.second_begin;
    auto remaining = static_cast<std::int32_t>(
        group.second_end - group.second_begin) >> 2;
    while (remaining > 0) {
        const auto half = remaining / 2;
        const auto midpoint = position +
            static_cast<std::uint32_t>(half) * 4u;
        const auto object = runtime.ReadWord(midpoint);
        const auto record = runtime.GetNameRecord(object, 4u);
        if (CompareName(record, name, runtime) < 0) {
            position = midpoint + 4u;
            remaining -= half + 1;
        } else {
            remaining = half;
        }
    }
    if (position == group.second_end) return 0u;
    const auto object = runtime.ReadWord(position);
    const auto record = runtime.GetNameRecord(object, 4u);
    return CompareName(record, name, runtime) == 0 ? object : 0u;
}

}  // namespace pal4
