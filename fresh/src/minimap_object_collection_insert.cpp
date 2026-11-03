#include "pal4/minimap_object_collection.h"

#include <algorithm>
#include <bit>

namespace pal4 {

std::uint32_t MinimapObjectCollection::InsertGroups(
    const std::uint32_t collection_address,
    MinimapObjectCollectionState& state,
    const std::uint32_t insertion_address,
    const std::uint32_t count,
    const std::uint32_t source_address,
    IMinimapObjectInsertRuntime& runtime) {
    const auto old_begin = state.ReadDword(4u);
    const auto old_end = state.ReadDword(8u);
    const auto old_capacity = state.ReadDword(12u);
    const auto free_slots = (old_capacity - old_end) / 20u;
    const auto inserted_bytes = count * 20u;
    if (free_slots < count) {
        const auto old_count = old_begin == 0u ? 0u :
            (old_end - old_begin) / 20u;
        const auto new_capacity = old_count +
            (old_begin == 0u ? count : std::max(old_count, count));
        const auto allocation_count =
            std::bit_cast<std::int32_t>(new_capacity) < 0 ?
                0u : new_capacity;
        const auto replacement = runtime.Allocate(
            allocation_count * 20u);
        auto destination = replacement;
        for (auto source = old_begin; source != insertion_address;
             source += 20u, destination += 20u) {
            runtime.CopyObject(destination, source);
        }
        for (std::uint32_t index = 0u; index < count;
             ++index, destination += 20u) {
            runtime.CopyObject(destination, source_address);
        }
        for (auto source = insertion_address; source != old_end;
             source += 20u, destination += 20u) {
            runtime.CopyObject(destination, source);
        }
        for (auto source = old_begin; source != old_end;
             source += 20u) {
            runtime.CleanupGroup(source);
        }
        runtime.Free(old_begin);
        state.WriteDword(12u, replacement + new_capacity * 20u);
        state.WriteDword(4u, replacement);
        state.WriteDword(8u, replacement +
                          (old_count + count) * 20u);
        return old_begin;
    }

    const auto trailing_count = (old_end - insertion_address) / 20u;
    if (trailing_count < count) {
        for (auto source = insertion_address; source != old_end;
             source += 20u) {
            runtime.CopyObject(source + inserted_bytes, source);
        }
        auto destination = old_end;
        for (auto index = trailing_count; index < count; ++index) {
            runtime.CopyObject(destination, source_address);
            destination += 20u;
        }
        for (auto destination_at = insertion_address;
             destination_at != old_end; destination_at += 20u) {
            runtime.WriteDword(destination_at,
                               runtime.ReadDword(source_address));
            runtime.AssignNestedArray(destination_at + 4u,
                                      source_address + 4u);
        }
        state.WriteDword(8u, old_end + inserted_bytes);
        return collection_address;
    }

    if (count == 0u) return 0u;
    for (auto source = old_end - inserted_bytes; source != old_end;
         source += 20u) {
        runtime.CopyObject(source + inserted_bytes, source);
    }
    for (auto source = old_end - inserted_bytes;
         source != insertion_address;) {
        source -= 20u;
        runtime.WriteDword(source + inserted_bytes,
                           runtime.ReadDword(source));
        runtime.AssignNestedArray(source + inserted_bytes + 4u,
                                  source + 4u);
    }
    for (auto destination = insertion_address;
         destination != insertion_address + inserted_bytes;
         destination += 20u) {
        runtime.WriteDword(destination,
                           runtime.ReadDword(source_address));
        runtime.AssignNestedArray(destination + 4u,
                                  source_address + 4u);
    }
    state.WriteDword(8u, old_end + inserted_bytes);
    return inserted_bytes;
}

void MinimapObjectCollection::CopyGroup(
    const std::uint32_t destination,
    const std::uint32_t source,
    IMinimapObjectGroupRuntime& runtime) {
    if (destination == 0u) return;
    runtime.WriteDword(destination, runtime.ReadDword(source));
    runtime.WriteByte(destination + 4u,
                      runtime.ReadByte(source + 4u));
    const auto source_begin = runtime.ReadDword(source + 8u);
    std::uint32_t count = 0u;
    if (source_begin != 0u) {
        const auto source_end = runtime.ReadDword(source + 12u);
        const auto signed_count =
            std::bit_cast<std::int32_t>(source_end - source_begin) >> 2;
        if (signed_count > 0) {
            count = static_cast<std::uint32_t>(signed_count);
        }
    }
    const auto allocation = runtime.Allocate(count * 4u);
    runtime.WriteDword(destination + 8u, allocation);
    const auto source_end = runtime.ReadDword(source + 12u);
    const auto copied_begin = runtime.ReadDword(source + 8u);
    auto target = allocation;
    for (auto pointer = copied_begin; pointer != source_end;
         pointer += 4u, target += 4u) {
        if (target != 0u) {
            runtime.WriteDword(target, runtime.ReadDword(pointer));
        }
    }
    runtime.WriteDword(destination + 12u, target);
    runtime.WriteDword(destination + 16u, target);
}

}  // namespace pal4
