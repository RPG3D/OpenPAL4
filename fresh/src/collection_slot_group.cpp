#include "pal4/collection_slot_group.h"

#include <new>

namespace pal4 {

std::uint32_t CollectionSlotGroup::Construct(
    const std::uint32_t object, const std::uint8_t seed,
    const std::uint8_t element_seed, ICollectionSlotGroupRuntime& runtime) {
    runtime.WriteWord(object + 4u, 1u);
    runtime.WriteWord(object, kVtable);
    for (std::uint32_t index = 0; index < 7u; ++index) {
        InitializeElement(object + 8u + index * 16u, element_seed, runtime);
    }
    runtime.WriteWord(object + 0x7cu, 0u);
    runtime.WriteByte(object + 0x78u, seed);
    runtime.WriteWord(object + 0x80u, 0u);
    runtime.WriteWord(object + 0x84u, 0u);
    return object;
}

void CollectionSlotGroup::InitializeElement(
    const std::uint32_t element, const std::uint8_t seed,
    ICollectionSlotGroupRuntime& runtime) {
    runtime.WriteByte(element, seed);
    runtime.WriteWord(element + 4u, 0u);
    runtime.WriteWord(element + 8u, 0u);
    runtime.WriteWord(element + 12u, 0u);
}

std::uint32_t CollectionSlotGroup::ClearEntries(
    const std::uint32_t object, ICollectionSlotGroupRuntime& runtime) {
    for (std::uint32_t index = 0; index < 7u; ++index) {
        const auto element = object + 8u + index * 16u;
        const auto begin = runtime.ReadWord(element + 4u);
        const auto end = runtime.ReadWord(element + 8u);
        for (auto cursor = begin; cursor != end; cursor += 4u) {
            const auto entry = runtime.ReadWord(cursor);
            if (entry != 0u) {
                runtime.DestroyEntry(entry);
                runtime.FreeMemory(entry);
            }
        }
        runtime.WriteWord(element + 8u, begin);
    }
    const auto old_end = runtime.ReadWord(object + 0x80u);
    runtime.WriteWord(object + 0x80u, runtime.ReadWord(object + 0x7cu));
    return old_end;
}

void CollectionSlotGroup::Destroy(
    const std::uint32_t object, ICollectionSlotGroupRuntime& runtime) {
    (void)ClearEntries(object, runtime);
    runtime.FreeMemory(runtime.ReadWord(object + 0x7cu));
    runtime.WriteWord(object + 0x7cu, 0u);
    runtime.WriteWord(object + 0x80u, 0u);
    runtime.WriteWord(object + 0x84u, 0u);
    for (std::uint32_t index = 7u; index != 0u; --index) {
        const auto element = object + 8u + (index - 1u) * 16u;
        runtime.FreeMemory(runtime.ReadWord(element + 4u));
        runtime.WriteWord(element + 4u, 0u);
        runtime.WriteWord(element + 8u, 0u);
        runtime.WriteWord(element + 12u, 0u);
    }
    runtime.WriteWord(object + 4u, 1u);
    runtime.WriteWord(object, kBaseVtable);
}

std::uint32_t CollectionSlotGroup::ProcessGameData(
    const std::uint32_t object, const std::uint32_t key,
    ICollectionSlotGroupProcessRuntime& runtime) {
    const auto source = runtime.FindGameData(key);
    if (source == 0u) return 0u;
    const auto slot = SlotAddress(
        object, static_cast<std::int32_t>(runtime.ReadWord(source + 148u)),
        runtime);
    if (slot == 0u) return 0u;

    const auto snapshot = runtime.AllocateMemory(0x194u);
    if (snapshot != 0u) {
        const auto sequence = runtime.ReadWord(object + 4u);
        runtime.WriteWord(object + 4u, sequence + 1u);
        runtime.CopyItemSnapshot(snapshot, source);
        runtime.WriteWord(snapshot + 400u, sequence);
    }
    runtime.AppendToSlot(slot, snapshot);
    runtime.InsertGlobalData(key, snapshot);
    if (snapshot == 0u) throw std::bad_alloc();

    auto begin = runtime.ReadWord(object + 0x7cu);
    auto end = runtime.ReadWord(object + 0x80u);
    auto capacity = runtime.ReadWord(object + 0x84u);
    if (begin != 0u && (end - begin) / 4u >= 6u) {
        for (auto cursor = begin + 4u; cursor != end; cursor += 4u) {
            runtime.WriteWord(cursor - 4u, runtime.ReadWord(cursor));
        }
        end -= 4u;
        runtime.WriteWord(object + 0x80u, end);
    }

    const auto sequence = runtime.ReadWord(snapshot + 400u);
    if (end != capacity) {
        runtime.WriteWord(end, sequence);
        runtime.WriteWord(object + 0x80u, end + 4u);
    } else {
        const auto size = begin != 0u ? (end - begin) / 4u : 0u;
        const auto new_capacity = size + (size <= 1u ? 1u : size);
        const auto storage = runtime.AllocateMemory(new_capacity * 4u);
        if (storage == 0u) throw std::bad_alloc();
        for (std::uint32_t index = 0u; index < size; ++index) {
            runtime.WriteWord(storage + index * 4u,
                              runtime.ReadWord(begin + index * 4u));
        }
        runtime.WriteWord(storage + size * 4u, sequence);
        runtime.FreeMemory(begin);
        runtime.WriteWord(object + 0x7cu, storage);
        runtime.WriteWord(object + 0x80u, storage + (size + 1u) * 4u);
        runtime.WriteWord(object + 0x84u, storage + new_capacity * 4u);
    }
    return snapshot;
}

bool CollectionSlotGroup::RemoveGameData(
    const std::uint32_t object, const std::uint32_t snapshot,
    ICollectionSlotGroupProcessRuntime& runtime) {
    if (snapshot == 0u) return false;
    const auto slot = SlotAddress(
        object, static_cast<std::int32_t>(runtime.ReadWord(snapshot + 148u)),
        runtime);
    if (slot == 0u) return false;

    const auto sequence = runtime.ReadWord(snapshot + 400u);
    const auto queue_begin = runtime.ReadWord(object + 0x7cu);
    auto queue_end = runtime.ReadWord(object + 0x80u);
    for (auto cursor = queue_begin; cursor != queue_end; cursor += 4u) {
        if (runtime.ReadWord(cursor) == sequence) {
            for (auto next = cursor + 4u; next != queue_end; next += 4u) {
                runtime.WriteWord(next - 4u, runtime.ReadWord(next));
            }
            queue_end -= 4u;
            runtime.WriteWord(object + 0x80u, queue_end);
            break;
        }
    }

    const auto begin = runtime.ReadWord(slot + 4u);
    auto end = runtime.ReadWord(slot + 8u);
    const auto lower = LowerBoundSequence(
        begin, end, static_cast<std::int32_t>(sequence), runtime);
    if (lower == end ||
        static_cast<std::int32_t>(sequence) <
            static_cast<std::int32_t>(
                runtime.ReadWord(runtime.ReadWord(lower) + 400u))) {
        return false;
    }
    const auto entry = runtime.ReadWord(lower);
    if (entry != 0u) {
        runtime.DestroyEntry(entry);
        runtime.FreeMemory(entry);
    }
    for (auto next = lower + 4u; next != end; next += 4u) {
        runtime.WriteWord(next - 4u, runtime.ReadWord(next));
    }
    end -= 4u;
    runtime.WriteWord(slot + 8u, end);
    return true;
}

std::uint32_t CollectionSlotGroup::SlotAddress(
    const std::uint32_t object, const std::int32_t index,
    ICollectionSlotGroupRuntime& runtime) {
    if (index < 0) {
        runtime.ReportInvalidSlot(index, 0xcdu);
        return 0u;
    }
    if (index >= 7) {
        runtime.ReportInvalidSlot(index, 0xd4u);
        return 0u;
    }
    return object + 8u + static_cast<std::uint32_t>(index) * 16u;
}

std::uint32_t CollectionSlotGroup::SearchSlot(
    const std::uint32_t slot, const std::int32_t sequence,
    ICollectionSlotGroupRuntime& runtime) {
    const auto begin = runtime.ReadWord(slot + 4u);
    const auto end = runtime.ReadWord(slot + 8u);
    auto lower = begin;
    auto count = (end - begin) / 4u;
    while (count != 0u) {
        const auto half = count / 2u;
        const auto middle = lower + half * 4u;
        const auto entry = runtime.ReadWord(middle);
        const auto value = static_cast<std::int32_t>(
            runtime.ReadWord(entry + 400u));
        if (value < sequence) {
            lower = middle + 4u;
            count -= half + 1u;
        } else {
            count = half;
        }
    }
    if (lower == end) return 0u;
    const auto entry = runtime.ReadWord(lower);
    return static_cast<std::int32_t>(runtime.ReadWord(entry + 400u)) ==
                   sequence
               ? entry
               : 0u;
}

std::uint32_t CollectionSlotGroup::LowerBoundSequence(
    const std::uint32_t begin, const std::uint32_t end,
    const std::int32_t sequence, ICollectionSlotGroupRuntime& runtime) {
    auto lower = begin;
    auto count = static_cast<std::int32_t>(end - begin) >> 2;
    while (count > 0) {
        const auto half = count / 2;
        const auto middle = lower + static_cast<std::uint32_t>(half) * 4u;
        const auto entry = runtime.ReadWord(middle);
        const auto value = static_cast<std::int32_t>(
            runtime.ReadWord(entry + 400u));
        if (value < sequence) {
            lower = middle + 4u;
            count -= half + 1;
        } else {
            count = half;
        }
    }
    return lower;
}

std::uint32_t CollectionSlotGroup::FindSequence(
    const std::uint32_t object, const std::int32_t sequence,
    const std::int32_t index, ICollectionSlotGroupRuntime& runtime) {
    if (index >= 0 && index < 7) {
        return SearchSlot(SlotAddress(object, index, runtime), sequence,
                          runtime);
    }
    for (std::uint32_t current = 0u; current < 7u; ++current) {
        const auto found = SearchSlot(object + 8u + current * 16u,
                                      sequence, runtime);
        if (found != 0u) return found;
    }
    return 0u;
}

std::uint32_t CollectionSlotGroup::CountInSlot(
    const std::uint32_t object, const std::int32_t index,
    ICollectionSlotGroupRuntime& runtime) {
    const auto slot = SlotAddress(object, index, runtime);
    if (slot == 0u) return 0u;
    const auto begin = runtime.ReadWord(slot + 4u);
    return begin != 0u ?
        (runtime.ReadWord(slot + 8u) - begin) / 4u : 0u;
}

std::uint32_t CollectionSlotGroup::GetEntry(
    const std::uint32_t object, const std::int32_t index,
    const std::uint32_t offset, ICollectionSlotGroupRuntime& runtime) {
    const auto slot = SlotAddress(object, index, runtime);
    if (slot == 0u) return 0u;
    const auto begin = runtime.ReadWord(slot + 4u);
    if (begin == 0u ||
        (runtime.ReadWord(slot + 8u) - begin) / 4u <= offset) {
        return 0u;
    }
    return runtime.ReadWord(begin + offset * 4u);
}

}  // namespace pal4
