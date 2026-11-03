#include "pal4/inventory_slot_array.h"

#include <new>
#include <stdexcept>

namespace pal4 {

std::uint32_t InventorySlotArray::Construct(
    const std::uint32_t array, IInventorySlotArrayRuntime& runtime) {
    std::uint32_t constructed = 0u;
    try {
        for (; constructed < 3u; ++constructed)
            runtime.ConstructElement(array + constructed * 16u);
    } catch (...) {
        while (constructed != 0u) {
            --constructed;
            runtime.DestroyElement(array + constructed * 16u);
        }
        throw;
    }
    return array;
}

std::uint32_t InventorySlotArray::ResetEnds(
    const std::uint32_t array, IInventorySlotArrayRuntime& runtime) {
    std::uint32_t old_end = 0u;
    for (std::uint32_t index = 0u; index < 3u; ++index) {
        const auto element = array + index * 16u;
        old_end = runtime.ReadWord(element + 8u);
        runtime.WriteWord(element + 8u,
                          runtime.ReadWord(element + 4u));
    }
    return old_end;
}

void InventorySlotArray::Cleanup(
    const std::uint32_t array, IInventorySlotArrayRuntime& runtime) {
    (void)ResetEnds(array, runtime);
    for (std::uint32_t index = 3u; index != 0u; --index)
        runtime.DestroyElement(array + (index - 1u) * 16u);
}

std::uint32_t InventorySlotArray::SlotAddress(
    const std::uint32_t array, const std::int32_t index,
    IInventorySlotQueryRuntime& runtime) {
    if (index < 0) {
        runtime.LogInvalidIndex(index, 0x82u);
        return 0u;
    }
    if (index >= 3) {
        runtime.LogInvalidIndex(index, 0x89u);
        return 0u;
    }
    return array + static_cast<std::uint32_t>(index) * 16u;
}

std::uint32_t InventorySlotArray::InsertByIndex(
    const std::uint32_t array, const std::int32_t index,
    const std::uint32_t id, IInventorySlotQueryRuntime& runtime) {
    const auto slot = SlotAddress(array, index, runtime);
    return slot == 0u ? 0u : InsertSorted(slot, id, runtime);
}

std::uint32_t InventorySlotArray::InsertSorted(
    const std::uint32_t slot, const std::uint32_t id,
    IInventorySlotQueryRuntime& runtime) {
    const auto begin = runtime.ReadWord(slot + 4u);
    const auto end = runtime.ReadWord(slot + 8u);
    auto lower = begin;
    auto count = static_cast<std::int32_t>(end - begin) >> 2;
    const auto signed_id = static_cast<std::int32_t>(id);
    while (count > 0) {
        const auto half = count / 2;
        const auto middle = lower + static_cast<std::uint32_t>(half) * 4u;
        if (static_cast<std::int32_t>(runtime.ReadWord(middle)) < signed_id) {
            lower = middle + 4u;
            count -= half + 1;
        } else {
            count = half;
        }
    }
    if (lower != end &&
        static_cast<std::int32_t>(runtime.ReadWord(lower)) == signed_id)
        return id;

    const auto capacity = runtime.ReadWord(slot + 12u);
    if (static_cast<std::int32_t>(capacity - end) >= 4) {
        for (auto cursor = end; cursor != lower; cursor -= 4u)
            runtime.WriteWord(cursor, runtime.ReadWord(cursor - 4u));
        runtime.WriteWord(lower, id);
        runtime.WriteWord(slot + 8u, end + 4u);
        return lower;
    }

    const auto old_count = begin == 0u ? 0u : (end - begin) / 4u;
    const auto growth = old_count > 1u ? old_count : 1u;
    const auto new_capacity = old_count + growth;
    const auto replacement = runtime.AllocateMemory(new_capacity * 4u);
    if (replacement == 0u) throw std::bad_alloc();
    const auto insertion_index = begin == 0u ? 0u : (lower - begin) / 4u;
    for (std::uint32_t index = 0u; index < insertion_index; ++index)
        runtime.WriteWord(replacement + index * 4u,
                          runtime.ReadWord(begin + index * 4u));
    runtime.WriteWord(replacement + insertion_index * 4u, id);
    for (auto index = insertion_index; index < old_count; ++index)
        runtime.WriteWord(replacement + (index + 1u) * 4u,
                          runtime.ReadWord(begin + index * 4u));
    runtime.FreeMemory(begin);
    runtime.WriteWord(slot + 12u,
                      replacement + new_capacity * 4u);
    runtime.WriteWord(slot + 4u, replacement);
    const auto new_end = replacement + (old_count + 1u) * 4u;
    runtime.WriteWord(slot + 8u, new_end);
    return new_end;
}

std::uint8_t InventorySlotArray::SerializeAll(
    const std::uint32_t array, const std::uint32_t stream,
    IInventorySlotStreamRuntime& runtime) {
    std::uint8_t result = 0u;
    for (std::uint32_t index = 0u; index < 3u; ++index)
        result = SerializeSlot(array + index * 16u, stream, runtime);
    return result;
}

std::uint8_t InventorySlotArray::SerializeSlot(
    const std::uint32_t slot, const std::uint32_t stream,
    IInventorySlotStreamRuntime& runtime) {
    const auto begin = runtime.ReadWord(slot + 4u);
    const auto end = runtime.ReadWord(slot + 8u);
    const auto count = begin == 0u ? 0u : (end - begin) / 4u;
    runtime.WriteStreamWord(stream, count);
    for (auto cursor = begin; cursor != end; cursor += 4u)
        runtime.WriteStreamWord(stream, runtime.ReadWord(cursor));
    return 1u;
}

std::uint8_t InventorySlotArray::DeserializeAll(
    const std::uint32_t array, const std::uint32_t stream,
    IInventorySlotStreamRuntime& runtime) {
    std::uint8_t result = 0u;
    for (std::uint32_t index = 0u; index < 3u; ++index)
        result = DeserializeSlot(array + index * 16u, stream, runtime);
    return result;
}

std::uint8_t InventorySlotArray::DeserializeSlot(
    const std::uint32_t slot, const std::uint32_t stream,
    IInventorySlotStreamRuntime& runtime) {
    auto begin = runtime.ReadWord(slot + 4u);
    auto end = begin;
    auto capacity = runtime.ReadWord(slot + 12u);
    runtime.WriteWord(slot + 8u, begin);
    const auto count = static_cast<std::int32_t>(
        runtime.ReadStreamWord(stream));
    for (std::int32_t index = 0; index < count; ++index) {
        const auto value = runtime.ReadStreamWord(stream);
        if (static_cast<std::int32_t>(capacity - end) < 4) {
            const auto used = begin == 0u ? 0u : (end - begin) / 4u;
            const auto growth = used > 1u ? used : 1u;
            const auto new_capacity = used + growth;
            const auto replacement = runtime.AllocateMemory(
                new_capacity * 4u);
            if (replacement == 0u) throw std::bad_alloc();
            for (std::uint32_t old = 0u; old < used; ++old)
                runtime.WriteWord(replacement + old * 4u,
                                  runtime.ReadWord(begin + old * 4u));
            runtime.WriteWord(replacement + used * 4u, value);
            runtime.FreeMemory(begin);
            begin = replacement;
            end = replacement + (used + 1u) * 4u;
            capacity = replacement + new_capacity * 4u;
            runtime.WriteWord(slot + 12u, capacity);
            runtime.WriteWord(slot + 4u, begin);
        } else {
            runtime.WriteWord(end, value);
            end += 4u;
        }
        runtime.WriteWord(slot + 8u, end);
    }
    return 1u;
}

std::uint32_t InventorySlotArray::ProcessGameData(
    const std::uint32_t array, const std::uint32_t id,
    IInventorySlotQueryRuntime& runtime) {
    const auto record = runtime.FindGameData(
        runtime.GameDbInstance() + 128u, id);
    if (record == 0u) return 0u;
    const auto index = static_cast<std::int32_t>(
        runtime.ReadWord(record + 100u));
    return InsertByIndex(array, index, id, runtime);
}

bool InventorySlotArray::Contains(
    const std::uint32_t array, const std::int32_t id,
    IInventorySlotQueryRuntime& runtime) {
    const auto record = runtime.FindGameData(
        runtime.GameDbInstance() + 128u,
        static_cast<std::uint32_t>(id));
    if (record == 0u) return false;
    const auto slot = SlotAddress(
        array, static_cast<std::int32_t>(runtime.ReadWord(record + 100u)),
        runtime);
    if (slot == 0u) return false;
    auto lower = runtime.ReadWord(slot + 4u);
    const auto end = runtime.ReadWord(slot + 8u);
    auto count = static_cast<std::int32_t>(end - lower) >> 2;
    while (count > 0) {
        const auto half = count / 2;
        const auto middle = lower + static_cast<std::uint32_t>(half) * 4u;
        if (static_cast<std::int32_t>(runtime.ReadWord(middle)) < id) {
            lower = middle + 4u;
            count -= half + 1;
        } else {
            count = half;
        }
    }
    return lower != end &&
           static_cast<std::int32_t>(runtime.ReadWord(lower)) == id;
}

std::int32_t InventorySlotArray::Size(
    const std::uint32_t array, const std::int32_t index,
    IInventorySlotQueryRuntime& runtime) {
    const auto slot = SlotAddress(array, index, runtime);
    if (slot == 0u) return 0;
    const auto begin = runtime.ReadWord(slot + 4u);
    if (begin == 0u) return 0;
    return static_cast<std::int32_t>(
        runtime.ReadWord(slot + 8u) - begin) >> 2;
}

std::uint32_t InventorySlotArray::ValueAt(
    const std::uint32_t array, const std::int32_t index,
    const std::uint32_t offset, IInventorySlotQueryRuntime& runtime) {
    const auto slot = SlotAddress(array, index, runtime);
    if (slot == 0u) return 0u;
    const auto begin = runtime.ReadWord(slot + 4u);
    const auto count = begin == 0u ? 0u :
        static_cast<std::uint32_t>(
            static_cast<std::int32_t>(
                runtime.ReadWord(slot + 8u) - begin) >> 2);
    if (begin == 0u || count <= offset)
        ThrowInvalidSubscript();
    return runtime.ReadWord(begin + offset * 4u);
}

void InventorySlotArray::ThrowInvalidSubscript() {
    throw std::out_of_range("invalid vector<T> subscript");
}

}  // namespace pal4
