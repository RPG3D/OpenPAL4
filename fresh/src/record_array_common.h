#pragma once

#include <algorithm>
#include <cstdint>

namespace pal4::detail {

template <std::uint32_t Stride, class Runtime>
std::uint32_t RecordArraySize(std::uint32_t array, Runtime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    return begin ? (runtime.ReadWord(array + 8) - begin) / Stride : 0;
}

template <std::uint32_t Stride, class Runtime>
std::uint32_t CopyRecordRange(std::uint32_t begin, std::uint32_t end,
                              std::uint32_t destination, Runtime& runtime) {
    for (auto item = begin; item != end;
         item += Stride, destination += Stride)
        static_cast<void>(runtime.CopyRecord(destination, item));
    return destination;
}

template <std::uint32_t Stride, class Runtime>
std::uint32_t InsertRecord(std::uint32_t array, std::uint32_t position,
                           std::uint32_t source, Runtime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    const auto end = runtime.ReadWord(array + 8);
    const auto capacity_end = runtime.ReadWord(array + 12);
    const auto insertion_index = (position - begin) / Stride;
    if ((capacity_end - end) / Stride != 0) {
        if (position != end) {
            static_cast<void>(runtime.CopyRecord(end, end - Stride));
            for (auto item = end - Stride; item != position;) {
                item -= Stride;
                runtime.AssignRecord(item + Stride, item);
            }
            runtime.AssignRecord(position, source);
        } else {
            static_cast<void>(runtime.CopyRecord(end, source));
        }
        runtime.WriteWord(array + 8, end + Stride);
    } else {
        const auto old_size = begin ? (end - begin) / Stride : 0;
        const auto new_capacity = old_size + std::max(old_size, 1u);
        const auto new_begin = runtime.Allocate(new_capacity * Stride);
        auto destination = CopyRecordRange<Stride>(begin, position,
                                                    new_begin, runtime);
        static_cast<void>(runtime.CopyRecord(destination, source));
        destination += Stride;
        static_cast<void>(CopyRecordRange<Stride>(position, end,
                                                  destination, runtime));
        for (auto item = begin; item != end; item += Stride)
            runtime.DestroyRecord(item);
        runtime.Free(begin);
        runtime.WriteWord(array + 12, new_begin + new_capacity * Stride);
        runtime.WriteWord(array + 4, new_begin);
        runtime.WriteWord(array + 8, new_begin + (old_size + 1) * Stride);
    }
    return runtime.ReadWord(array + 4) + insertion_index * Stride;
}

template <std::uint32_t Stride, class Runtime>
void InsertRecordCopies(std::uint32_t array, std::uint32_t position,
                        std::uint32_t count, std::uint32_t source,
                        Runtime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    const auto end = runtime.ReadWord(array + 8);
    const auto capacity_end = runtime.ReadWord(array + 12);
    const auto inserted_bytes = count * Stride;
    const auto spare = (capacity_end - end) / Stride;
    if (spare >= count) {
        const auto tail = (end - position) / Stride;
        if (tail >= count) {
            if (count == 0) return;
            static_cast<void>(CopyRecordRange<Stride>(
                end - inserted_bytes, end, end, runtime));
            for (auto item = end - inserted_bytes; item != position;) {
                item -= Stride;
                runtime.AssignRecord(item + inserted_bytes, item);
            }
            for (auto item = position; item != position + inserted_bytes;
                 item += Stride)
                runtime.AssignRecord(item, source);
        } else {
            static_cast<void>(CopyRecordRange<Stride>(
                position, end, position + inserted_bytes, runtime));
            for (auto item = end; item != end + (count - tail) * Stride;
                 item += Stride)
                static_cast<void>(runtime.CopyRecord(item, source));
            for (auto item = position; item != end; item += Stride)
                runtime.AssignRecord(item, source);
        }
        runtime.WriteWord(array + 8, end + inserted_bytes);
        return;
    }

    const auto used = begin ? (end - begin) / Stride : 0;
    const auto growth = std::max(used, count);
    const auto new_capacity = used + growth;
    const auto allocation_count = static_cast<std::int32_t>(new_capacity) < 0
        ? 0u : new_capacity;
    const auto new_begin = runtime.Allocate(allocation_count * Stride);
    auto destination = CopyRecordRange<Stride>(begin, position,
                                                new_begin, runtime);
    for (std::uint32_t index = 0; index < count; ++index) {
        static_cast<void>(runtime.CopyRecord(destination, source));
        destination += Stride;
    }
    static_cast<void>(CopyRecordRange<Stride>(position, end, destination,
                                              runtime));
    for (auto item = begin; item != end; item += Stride)
        runtime.DestroyRecord(item);
    runtime.Free(begin);
    runtime.WriteWord(array + 12, new_begin + new_capacity * Stride);
    runtime.WriteWord(array + 4, new_begin);
    runtime.WriteWord(array + 8, new_begin + (used + count) * Stride);
}

}  // namespace pal4::detail
