#include "pal4/combat_event_collection.h"

#include <algorithm>

namespace pal4 {
namespace {

constexpr std::uint32_t kRecordBytes = 20u;

void AssignRecord(std::uint32_t destination, std::uint32_t source,
                  ICombatEventCollectionRuntime& runtime) {
    runtime.AssignText(destination, source);
    runtime.WriteWord(destination + 16u, runtime.ReadWord(source + 16u));
}

}  // namespace

std::uint32_t CombatEventCollection::InsertCopies(
    std::uint32_t collection, std::uint32_t position,
    std::uint32_t count, std::uint32_t source,
    ICombatEventCollectionRuntime& runtime) {
    const auto begin = runtime.ReadWord(collection + 4u);
    const auto end = runtime.ReadWord(collection + 8u);
    const auto capacity_end = runtime.ReadWord(collection + 12u);
    const auto bytes = count * kRecordBytes;
    const auto tail_count = (end - position) / kRecordBytes;

    if ((capacity_end - end) / kRecordBytes >= count) {
        if (tail_count < count) {
            for (auto at = position; at != end; at += kRecordBytes)
                runtime.CopyConstruct(at + bytes, at);
            for (auto at = end; at != end + (count - tail_count) * kRecordBytes;
                 at += kRecordBytes)
                runtime.CopyConstruct(at, source);
            for (auto at = position; at != end; at += kRecordBytes)
                AssignRecord(at, source, runtime);
            runtime.WriteWord(collection + 8u, end + bytes);
            return collection;
        }

        if (count == 0u) return 0u;
        for (auto at = end - bytes; at != end; at += kRecordBytes)
            runtime.CopyConstruct(at + bytes, at);
        for (auto at = end - bytes; at != position;) {
            at -= kRecordBytes;
            AssignRecord(at + bytes, at, runtime);
        }
        for (auto at = position; at != position + bytes; at += kRecordBytes)
            AssignRecord(at, source, runtime);
        runtime.WriteWord(collection + 8u, end + bytes);
        return bytes;
    }

    const auto used = begin ? (end - begin) / kRecordBytes : 0u;
    const auto capacity = used + std::max(used, count);
    const auto allocation_count =
        static_cast<std::int32_t>(capacity) < 0 ? 0u : capacity;
    const auto replacement = runtime.Allocate(allocation_count * kRecordBytes);
    auto destination = replacement;
    for (auto at = begin; at != position; at += kRecordBytes) {
        runtime.CopyConstruct(destination, at);
        destination += kRecordBytes;
    }
    for (std::uint32_t i = 0; i < count; ++i) {
        runtime.CopyConstruct(destination, source);
        destination += kRecordBytes;
    }
    for (auto at = position; at != end; at += kRecordBytes) {
        runtime.CopyConstruct(destination, at);
        destination += kRecordBytes;
    }
    for (auto at = begin; at != end; at += kRecordBytes)
        runtime.ReleaseText(at);
    runtime.Free(begin);
    runtime.WriteWord(collection + 12u, replacement + capacity * kRecordBytes);
    runtime.WriteWord(collection + 4u, replacement);
    runtime.WriteWord(collection + 8u, replacement + (used + count) * kRecordBytes);
    return 0u;
}

}  // namespace pal4
