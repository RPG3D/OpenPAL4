#include "pal4/utilities_text_record_table.h"
#include "pal4/utilities_preview_info_table.h"
#include "pal4/utilities_scene_table.h"
#include "pal4/utilities_window_table.h"
#include "pal4/utilities_world_table.h"

#include <algorithm>

namespace pal4 {
namespace {

std::uint32_t CopyRange(std::uint32_t begin, std::uint32_t end,
                        std::uint32_t destination, std::uint32_t stride,
                        IUtilitiesRecordArrayRuntime& runtime) {
    for (auto source = begin; source != end;
         source += stride, destination += stride)
        runtime.CopyConstruct(destination, source, stride);
    return destination;
}

std::uint32_t Count(std::uint32_t array, std::uint32_t stride,
                    IUtilitiesRecordArrayRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    return begin ? (runtime.ReadWord(array + 8) - begin) / stride : 0;
}

std::uint32_t Insert(std::uint32_t array, std::uint32_t position,
                     std::uint32_t source, std::uint32_t stride,
                     IUtilitiesRecordArrayRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    const auto end = runtime.ReadWord(array + 8);
    const auto capacity_end = runtime.ReadWord(array + 12);
    const auto index = (position - begin) / stride;
    if ((capacity_end - end) / stride != 0) {
        if ((end - position) / stride != 0) {
            runtime.CopyConstruct(end, end - stride, stride);
            runtime.MoveBackward(position, end - stride, end, stride);
            runtime.Assign(position, source, stride);
        } else {
            runtime.CopyConstruct(end, source, stride);
        }
        runtime.WriteWord(array + 8, end + stride);
        return begin + index * stride;
    }

    const auto used = begin ? (end - begin) / stride : 0;
    const auto growth = used > 1 ? used : 1;
    const auto new_capacity = used + growth;
    const auto replacement = runtime.Allocate(new_capacity * stride);
    auto destination = CopyRange(begin, position, replacement,
                                 stride, runtime);
    runtime.CopyConstruct(destination, source, stride);
    destination += stride;
    static_cast<void>(CopyRange(position, end, destination,
                                stride, runtime));
    runtime.DestroyRange(begin, end, stride);
    runtime.Free(begin);
    runtime.WriteWord(array + 12,
                      replacement + new_capacity * stride);
    runtime.WriteWord(array + 4, replacement);
    runtime.WriteWord(array + 8,
                      replacement + (used + 1) * stride);
    return replacement + index * stride;
}

void InsertRepeated(std::uint32_t array, std::uint32_t position,
                    std::uint32_t count, std::uint32_t source,
                    std::uint32_t stride,
                    IUtilitiesRecordArrayRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    const auto end = runtime.ReadWord(array + 8);
    const auto capacity_end = runtime.ReadWord(array + 12);
    const auto inserted_bytes = count * stride;
    const auto spare = (capacity_end - end) / stride;
    if (spare >= count) {
        const auto tail = (end - position) / stride;
        if (tail >= count) {
            if (count == 0) return;
            static_cast<void>(CopyRange(end - inserted_bytes, end, end,
                                        stride, runtime));
            runtime.MoveBackward(position, end - inserted_bytes,
                                 end, stride);
            for (auto at = position; at != position + inserted_bytes;
                 at += stride)
                runtime.Assign(at, source, stride);
        } else {
            static_cast<void>(CopyRange(position, end,
                                        position + inserted_bytes,
                                        stride, runtime));
            for (auto at = end; at != end + (count - tail) * stride;
                 at += stride)
                runtime.CopyConstruct(at, source, stride);
            for (auto at = position; at != end; at += stride)
                runtime.Assign(at, source, stride);
        }
        runtime.WriteWord(array + 8, end + inserted_bytes);
        return;
    }

    const auto used = begin ? (end - begin) / stride : 0;
    const auto growth = std::max(used, count);
    const auto new_capacity = used + growth;
    const auto allocation_count =
        static_cast<std::int32_t>(new_capacity) < 0 ? 0u : new_capacity;
    const auto replacement = runtime.Allocate(allocation_count * stride);
    auto destination = CopyRange(begin, position, replacement,
                                 stride, runtime);
    for (std::uint32_t i = 0; i < count; ++i) {
        runtime.CopyConstruct(destination, source, stride);
        destination += stride;
    }
    static_cast<void>(CopyRange(position, end, destination,
                                stride, runtime));
    runtime.DestroyRange(begin, end, stride);
    runtime.Free(begin);
    runtime.WriteWord(array + 12, replacement + new_capacity * stride);
    runtime.WriteWord(array + 4, replacement);
    runtime.WriteWord(array + 8, replacement + (used + count) * stride);
}

void FillRepeated(std::uint32_t destination, std::uint32_t count,
                  std::uint32_t source, std::uint32_t stride,
                  IUtilitiesRecordArrayRuntime& runtime) {
    for (std::uint32_t index = 0; index < count; ++index)
        runtime.CopyConstruct(destination + index * stride, source,
                              stride);
}

}  // namespace

std::uint32_t UtilitiesTextRecordArray::Insert(
    std::uint32_t array, std::uint32_t position, std::uint32_t source,
    IUtilitiesRecordArrayRuntime& runtime) {
    return pal4::Insert(array, position, source, 20, runtime);
}

std::uint32_t UtilitiesTextRecordArray::Count(
    std::uint32_t array, IUtilitiesRecordArrayRuntime& runtime) {
    return pal4::Count(array, 20, runtime);
}

std::uint32_t UtilitiesTextRecordArray::CopyRange(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination,
    IUtilitiesRecordArrayRuntime& runtime) {
    return pal4::CopyRange(begin, end, destination, 20, runtime);
}

void UtilitiesTextRecordArray::InsertCopies(
    std::uint32_t array, std::uint32_t position,
    std::uint32_t count, std::uint32_t source,
    IUtilitiesRecordArrayRuntime& runtime) {
    InsertRepeated(array, position, count, source, 20, runtime);
}

void UtilitiesTextRecordArray::Fill(
    std::uint32_t destination, std::uint32_t count,
    std::uint32_t source, IUtilitiesRecordArrayRuntime& runtime) {
    FillRepeated(destination, count, source, 20, runtime);
}

std::uint32_t UtilitiesPreviewInfoArray::Insert(
    std::uint32_t array, std::uint32_t position, std::uint32_t source,
    IUtilitiesRecordArrayRuntime& runtime) {
    return pal4::Insert(array, position, source, 52, runtime);
}

std::uint32_t UtilitiesPreviewInfoArray::Count(
    std::uint32_t array, IUtilitiesRecordArrayRuntime& runtime) {
    return pal4::Count(array, 52, runtime);
}

std::uint32_t UtilitiesPreviewInfoArray::CopyRange(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination,
    IUtilitiesRecordArrayRuntime& runtime) {
    return pal4::CopyRange(begin, end, destination, 52, runtime);
}

void UtilitiesPreviewInfoArray::InsertCopies(
    std::uint32_t array, std::uint32_t position,
    std::uint32_t count, std::uint32_t source,
    IUtilitiesRecordArrayRuntime& runtime) {
    InsertRepeated(array, position, count, source, 52, runtime);
}

void UtilitiesPreviewInfoArray::Fill(
    std::uint32_t destination, std::uint32_t count,
    std::uint32_t source, IUtilitiesRecordArrayRuntime& runtime) {
    FillRepeated(destination, count, source, 52, runtime);
}

std::uint32_t UtilitiesSceneArray::Insert(
    std::uint32_t array, std::uint32_t position, std::uint32_t source,
    IUtilitiesRecordArrayRuntime& runtime) {
    return pal4::Insert(array, position, source, 60, runtime);
}

std::uint32_t UtilitiesSceneArray::CopyRange(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination,
    IUtilitiesRecordArrayRuntime& runtime) {
    return pal4::CopyRange(begin, end, destination, 60, runtime);
}

void UtilitiesSceneArray::InsertCopies(
    std::uint32_t array, std::uint32_t position,
    std::uint32_t count, std::uint32_t source,
    IUtilitiesRecordArrayRuntime& runtime) {
    InsertRepeated(array, position, count, source, 60, runtime);
}

void UtilitiesSceneArray::Fill(
    std::uint32_t destination, std::uint32_t count,
    std::uint32_t source, IUtilitiesRecordArrayRuntime& runtime) {
    FillRepeated(destination, count, source, 60, runtime);
}

std::uint32_t UtilitiesWindowArray::Insert(
    std::uint32_t array, std::uint32_t position, std::uint32_t source,
    IUtilitiesRecordArrayRuntime& runtime) {
    return pal4::Insert(array, position, source, 44, runtime);
}

std::uint32_t UtilitiesWindowArray::CopyRange(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination,
    IUtilitiesRecordArrayRuntime& runtime) {
    return pal4::CopyRange(begin, end, destination, 44, runtime);
}

void UtilitiesWindowArray::InsertCopies(
    std::uint32_t array, std::uint32_t position,
    std::uint32_t count, std::uint32_t source,
    IUtilitiesRecordArrayRuntime& runtime) {
    InsertRepeated(array, position, count, source, 44, runtime);
}

void UtilitiesWindowArray::Fill(
    std::uint32_t destination, std::uint32_t count,
    std::uint32_t source, IUtilitiesRecordArrayRuntime& runtime) {
    FillRepeated(destination, count, source, 44, runtime);
}

std::uint32_t UtilitiesWorldArray::Insert(
    std::uint32_t array, std::uint32_t position, std::uint32_t source,
    IUtilitiesRecordArrayRuntime& runtime) {
    return pal4::Insert(array, position, source, 40, runtime);
}

std::uint32_t UtilitiesWorldArray::CopyRange(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination,
    IUtilitiesRecordArrayRuntime& runtime) {
    return pal4::CopyRange(begin, end, destination, 40, runtime);
}

void UtilitiesWorldArray::InsertCopies(
    std::uint32_t array, std::uint32_t position,
    std::uint32_t count, std::uint32_t source,
    IUtilitiesRecordArrayRuntime& runtime) {
    InsertRepeated(array, position, count, source, 40, runtime);
}

void UtilitiesWorldArray::Fill(
    std::uint32_t destination, std::uint32_t count,
    std::uint32_t source, IUtilitiesRecordArrayRuntime& runtime) {
    FillRepeated(destination, count, source, 40, runtime);
}

}  // namespace pal4
