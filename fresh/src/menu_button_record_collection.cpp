#include "pal4/menu_button_record_collection.h"

#include <algorithm>
#include <cstdint>

namespace pal4 {

std::uint32_t MenuButtonRecordCollection::Clear(
    const std::uint32_t collection,
    IMenuButtonRecordRuntime& runtime) {
    const auto begin = runtime.ReadWord(collection + 4u);
    const auto end = runtime.ReadWord(collection + 8u);
    for (auto record = begin; record != end; record += kRecordBytes)
        runtime.Cleanup(record);
    runtime.Free(begin);
    runtime.WriteWord(collection + 4u, 0u);
    runtime.WriteWord(collection + 8u, 0u);
    runtime.WriteWord(collection + 12u, 0u);
    return 0u;
}

void MenuButtonRecordCollection::Insert(
    const std::uint32_t collection,
    const std::uint32_t insertion, const std::uint32_t count,
    const std::uint32_t source,
    IMenuButtonRecordRuntime& runtime) {
    const auto begin = runtime.ReadWord(collection + 4u);
    const auto end = runtime.ReadWord(collection + 8u);
    const auto capacity_end = runtime.ReadWord(collection + 12u);
    const auto free_records = (capacity_end - end) / kRecordBytes;
    const auto tail_records = (end - insertion) / kRecordBytes;
    if (free_records >= count) {
        if (tail_records >= count) {
            if (count == 0u) return;
            for (std::uint32_t index = 0u; index < count; ++index) {
                runtime.CopyConstruct(end + index * kRecordBytes,
                                      end - count * kRecordBytes +
                                          index * kRecordBytes);
            }
            for (auto cursor = end - count * kRecordBytes;
                 cursor != insertion;) {
                cursor -= kRecordBytes;
                runtime.Assign(cursor + count * kRecordBytes, cursor);
            }
            for (std::uint32_t index = 0u; index < count; ++index)
                runtime.Assign(insertion + index * kRecordBytes, source);
        } else {
            for (std::uint32_t index = 0u;
                 index < tail_records; ++index) {
                runtime.CopyConstruct(
                    insertion + (count + index) * kRecordBytes,
                    insertion + index * kRecordBytes);
            }
            for (std::uint32_t index = tail_records;
                 index < count; ++index) {
                runtime.CopyConstruct(
                    insertion + index * kRecordBytes, source);
            }
            for (std::uint32_t index = 0u;
                 index < tail_records; ++index)
                runtime.Assign(insertion + index * kRecordBytes, source);
        }
        runtime.WriteWord(collection + 8u,
                          end + count * kRecordBytes);
        return;
    }

    const auto old_count = begin == 0u
        ? 0u : (end - begin) / kRecordBytes;
    const auto new_capacity = old_count + std::max(old_count, count);
    const auto new_begin = runtime.Allocate(new_capacity * kRecordBytes);
    const auto prefix_count = begin == 0u
        ? 0u : (insertion - begin) / kRecordBytes;
    for (std::uint32_t index = 0u; index < prefix_count; ++index)
        runtime.CopyConstruct(new_begin + index * kRecordBytes,
                              begin + index * kRecordBytes);
    for (std::uint32_t index = 0u; index < count; ++index)
        runtime.CopyConstruct(
            new_begin + (prefix_count + index) * kRecordBytes,
            source);
    for (std::uint32_t index = prefix_count;
         index < old_count; ++index)
        runtime.CopyConstruct(
            new_begin + (index + count) * kRecordBytes,
            begin + index * kRecordBytes);
    for (std::uint32_t index = 0u; index < old_count; ++index)
        runtime.Cleanup(begin + index * kRecordBytes);
    runtime.Free(begin);
    runtime.WriteWord(collection + 12u,
                      new_begin + new_capacity * kRecordBytes);
    runtime.WriteWord(collection + 4u, new_begin);
    runtime.WriteWord(collection + 8u,
                      new_begin + (old_count + count) * kRecordBytes);
}

}  // namespace pal4
