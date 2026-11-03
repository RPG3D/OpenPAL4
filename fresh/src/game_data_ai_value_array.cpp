#include "pal4/game_data_ai_value_array.h"

#include <new>

namespace pal4 {
namespace {

template <typename Runtime>
std::uint32_t CopyRange(std::uint32_t begin, std::uint32_t end,
                        std::uint32_t destination, std::uint32_t stride,
                        Runtime& runtime) {
    for (auto source = begin; source != end; source += stride) {
        if (destination)
            runtime.WriteRecord(destination, runtime.ReadRecord(source));
        destination += stride;
    }
    return destination;
}

template <typename Record, typename Runtime>
std::uint32_t FillRange(std::uint32_t destination, std::uint32_t count,
                        const Record& record, std::uint32_t stride,
                        Runtime& runtime) {
    if (count == 0) return 0;
    for (std::uint32_t i = 0; i < count; ++i) {
        if (destination) runtime.WriteRecord(destination, record);
        destination += stride;
    }
    return destination;
}

template <typename Record, typename Runtime>
std::uint32_t InsertRecord(std::uint32_t array, std::uint32_t position,
                           const Record& record, std::uint32_t stride,
                           Runtime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    const auto end = runtime.ReadWord(array + 8);
    const auto capacity_end = runtime.ReadWord(array + 12);
    const auto insertion_index = (position - begin) / stride;

    if ((capacity_end - end) / stride >= 1) {
        for (auto source = end; source != position; source -= stride)
            runtime.WriteRecord(source, runtime.ReadRecord(source - stride));
        runtime.WriteRecord(position, record);
        runtime.WriteWord(array + 8, end + stride);
        return begin + insertion_index * stride;
    }

    const auto old_count = begin ? (end - begin) / stride : 0;
    const auto growth = old_count > 1 ? old_count : 1;
    const auto new_capacity = old_count + growth;
    const auto replacement = runtime.Allocate(new_capacity * stride);
    if (!replacement) throw std::bad_alloc();
    auto destination = CopyRange(begin, position, replacement, stride, runtime);
    runtime.WriteRecord(destination, record);
    destination += stride;
    (void)CopyRange(position, end, destination, stride, runtime);
    runtime.Free(begin);
    runtime.WriteWord(array + 12, replacement + new_capacity * stride);
    runtime.WriteWord(array + 4, replacement);
    runtime.WriteWord(array + 8, replacement + (old_count + 1) * stride);
    return replacement + insertion_index * stride;
}

template <typename Record, typename Runtime>
std::uint32_t InsertCopies(std::uint32_t array, std::uint32_t position,
                           std::uint32_t count, const Record& record,
                           std::uint32_t stride, Runtime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    const auto end = runtime.ReadWord(array + 8);
    const auto capacity_end = runtime.ReadWord(array + 12);
    const auto free_count = (capacity_end - end) / stride;
    const auto suffix_count = (end - position) / stride;
    if (free_count >= count) {
        if (count == 0) return 0;
        for (auto source = end; source != position; source -= stride)
            runtime.WriteRecord(source + (count - 1) * stride,
                                runtime.ReadRecord(source - stride));
        for (std::uint32_t i = 0; i < count; ++i)
            runtime.WriteRecord(position + i * stride, record);
        const auto new_end = end + count * stride;
        runtime.WriteWord(array + 8, new_end);
        return suffix_count >= count ? position + count * stride : new_end;
    }

    const auto old_count = begin ? (end - begin) / stride : 0;
    const auto growth = old_count >= count ? old_count : count;
    const auto new_capacity = old_count + growth;
    const auto replacement = runtime.Allocate(new_capacity * stride);
    if (!replacement) throw std::bad_alloc();
    auto destination = CopyRange(begin, position, replacement, stride, runtime);
    for (std::uint32_t i = 0; i < count; ++i) {
        runtime.WriteRecord(destination, record);
        destination += stride;
    }
    (void)CopyRange(position, end, destination, stride, runtime);
    runtime.Free(begin);
    runtime.WriteWord(array + 12, replacement + new_capacity * stride);
    runtime.WriteWord(array + 4, replacement);
    runtime.WriteWord(array + 8, replacement + (old_count + count) * stride);
    return begin;
}

}  // namespace

std::uint32_t GameDataAiValueArray::Insert(
    std::uint32_t array, std::uint32_t position,
    const GameDataAiRecord& record, IGameDataAiValueArrayRuntime& runtime) {
    return InsertRecord(array, position, record, 56, runtime);
}

std::uint32_t GameDataAiValueArray::CopyRecords(
    std::uint32_t begin, std::uint32_t end, std::uint32_t destination,
    IGameDataAiValueArrayRuntime& runtime) {
    return CopyRange(begin, end, destination, 56, runtime);
}

std::uint32_t GameDataAiValueArray::InsertCopies(
    std::uint32_t array, std::uint32_t position, std::uint32_t count,
    const GameDataAiRecord& record, IGameDataAiValueArrayRuntime& runtime) {
    return pal4::InsertCopies(array, position, count, record, 56, runtime);
}

std::uint32_t GameDataAiValueArray::FillRecords(
    std::uint32_t destination, std::uint32_t count,
    const GameDataAiRecord& record, IGameDataAiValueArrayRuntime& runtime) {
    return FillRange(destination, count, record, 56, runtime);
}

void GameDataAiValueArray::CopyOne(
    std::uint32_t destination, std::uint32_t source,
    IGameDataAiValueArrayRuntime& runtime) {
    if (destination)
        runtime.WriteRecord(destination, runtime.ReadRecord(source));
}

std::uint32_t GameDataAiPropertyValueArray::Size(
    std::uint32_t array, IGameDataAiPropertyValueArrayRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    return begin ? (runtime.ReadWord(array + 8) - begin) / 68 : 0;
}

std::uint32_t GameDataAiPropertyValueArray::CopyRecords(
    std::uint32_t begin, std::uint32_t end, std::uint32_t destination,
    IGameDataAiPropertyValueArrayRuntime& runtime) {
    return CopyRange(begin, end, destination, 68, runtime);
}

std::uint32_t GameDataAiPropertyValueArray::Insert(
    std::uint32_t array, std::uint32_t position,
    const GameDataAiPropertyRecord& record,
    IGameDataAiPropertyValueArrayRuntime& runtime) {
    return InsertRecord(array, position, record, 68, runtime);
}

std::uint32_t GameDataAiPropertyValueArray::InsertCopies(
    std::uint32_t array, std::uint32_t position, std::uint32_t count,
    const GameDataAiPropertyRecord& record,
    IGameDataAiPropertyValueArrayRuntime& runtime) {
    return pal4::InsertCopies(array, position, count, record, 68, runtime);
}

std::uint32_t GameDataAiPropertyValueArray::FillRecords(
    std::uint32_t destination, std::uint32_t count,
    const GameDataAiPropertyRecord& record,
    IGameDataAiPropertyValueArrayRuntime& runtime) {
    return FillRange(destination, count, record, 68, runtime);
}

void GameDataAiPropertyValueArray::CopyOne(
    std::uint32_t destination, std::uint32_t source,
    IGameDataAiPropertyValueArrayRuntime& runtime) {
    if (destination)
        runtime.WriteRecord(destination, runtime.ReadRecord(source));
}

}  // namespace pal4
