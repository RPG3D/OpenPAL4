#include "pal4/indexed_sorted_value_arrays.h"

namespace pal4 {
namespace {

struct SearchResult {
    std::uint32_t address;
    std::uint32_t last_midpoint;
};

SearchResult LowerBound(const SortedValueArraySlot& slot, std::int32_t value,
                        IIndexedSortedValueArrayRuntime& runtime) {
    auto cursor = slot.begin;
    auto count = static_cast<std::int32_t>(slot.end - cursor) >> 2;
    std::uint32_t midpoint = 0u;
    while (count > 0) {
        midpoint = static_cast<std::uint32_t>(count / 2);
        const auto candidate = static_cast<std::int32_t>(
            runtime.ReadDword(cursor + midpoint * 4u));
        if (candidate >= value) {
            count /= 2;
        } else {
            cursor += (midpoint + 1u) * 4u;
            count -= static_cast<std::int32_t>(midpoint + 1u);
        }
    }
    return {cursor, midpoint};
}

}  // namespace

std::uint32_t DynamicDwordResultArray::Append(
    IndexedFilteredResultArrayState& result, std::uint32_t record,
    IDynamicDwordArrayRuntime& runtime) {
    if (result.end != result.capacity_end) {
        const auto old_end = result.end;
        runtime.WriteDword(result.end, record);
        result.end += 4u;
        return old_end;
    }
    const auto size = result.begin == 0u ? 0u :
                      (result.end - result.begin) / 4u;
    const auto capacity = size + (size > 1u ? size : 1u);
    const auto replacement = runtime.Allocate(capacity * 4u);
    for (std::uint32_t i = 0; i < size; ++i)
        runtime.WriteDword(replacement + i * 4u,
                           runtime.ReadDword(result.begin + i * 4u));
    runtime.WriteDword(replacement + size * 4u, record);
    runtime.Free(result.begin);
    result.begin = replacement;
    result.end = replacement + (size + 1u) * 4u;
    result.capacity_end = replacement + capacity * 4u;
    return result.end;
}

std::uint32_t IndexedSortedValueArrayRuntimeStub::ReadDword(std::uint32_t) {
    return 0u;
}
void IndexedSortedValueArrayRuntimeStub::WriteDword(
    std::uint32_t, std::uint32_t) {}
std::uint32_t IndexedSortedValueArrayRuntimeStub::Allocate(std::uint32_t) {
    return 0u;
}
void IndexedSortedValueArrayRuntimeStub::Free(std::uint32_t) {}
std::uint32_t IndexedSortedIteratorRuntimeStub::Allocate(std::uint32_t) {
    return 0u;
}
void IndexedSortedIteratorRuntimeStub::StoreValid(
    std::uint32_t, const IndexedSortedIteratorImage&) {}
void IndexedSortedIteratorRuntimeStub::StoreInvalid(
    std::uint32_t, const InvalidSortedIteratorImage&) {}
void IndexedSortedIteratorRuntimeStub::WriteOutput(
    std::uint32_t, std::uint32_t) {}
std::uint32_t SortedValueProcessRuntimeStub::ReadDword(std::uint32_t) {
    return 0u;
}
void SortedValueProcessRuntimeStub::Process(
    std::uint32_t, std::uint32_t) {}

std::uint32_t IndexedSortedValueArrays::InsertUnique(
    IndexedSortedValueArrayState& state, std::int32_t index,
    std::int32_t value, IIndexedSortedValueArrayRuntime& runtime) {
    if (index < 0 || index >= 6)
        return static_cast<std::uint32_t>(index);

    auto& slot = state.slots[static_cast<std::size_t>(index)];
    const auto found = LowerBound(slot, value, runtime);
    const auto position = found.address;
    if (position != slot.end &&
        runtime.ReadDword(position) == static_cast<std::uint32_t>(value))
        return found.last_midpoint;

    const auto old_size = slot.begin == 0u ? 0u :
                          (slot.end - slot.begin) / 4u;
    if (slot.capacity_end != slot.end) {
        const auto suffix = (slot.end - position) / 4u;
        for (auto source = slot.end; source > position; source -= 4u)
            runtime.WriteDword(source, runtime.ReadDword(source - 4u));
        runtime.WriteDword(position, static_cast<std::uint32_t>(value));
        slot.end += 4u;
        return suffix == 0u ? position : position + 4u;
    }

    const auto old_begin = slot.begin;
    const auto new_capacity = old_size + (old_size > 1u ? old_size : 1u);
    const auto replacement = runtime.Allocate(new_capacity * 4u);
    const auto prefix = old_begin == 0u ? 0u : (position - old_begin) / 4u;
    for (std::uint32_t i = 0; i < prefix; ++i)
        runtime.WriteDword(replacement + i * 4u,
                           runtime.ReadDword(old_begin + i * 4u));
    runtime.WriteDword(replacement + prefix * 4u,
                       static_cast<std::uint32_t>(value));
    for (auto i = prefix; i < old_size; ++i)
        runtime.WriteDword(replacement + (i + 1u) * 4u,
                           runtime.ReadDword(old_begin + i * 4u));
    runtime.Free(old_begin);
    slot.begin = replacement;
    slot.end = replacement + (old_size + 1u) * 4u;
    slot.capacity_end = replacement + new_capacity * 4u;
    return slot.end;
}

bool IndexedSortedValueArrays::Contains(
    const IndexedSortedValueArrayState& state, std::int32_t index,
    std::int32_t value, IIndexedSortedValueArrayRuntime& runtime) {
    if (index < 0 || index >= 6)
        return false;
    const auto& slot = state.slots[static_cast<std::size_t>(index)];
    const auto found = LowerBound(slot, value, runtime).address;
    return found != slot.end &&
           static_cast<std::int32_t>(runtime.ReadDword(found)) == value;
}

std::uint32_t IndexedSortedValueArrays::GetIteratorByIndex(
    const IndexedSortedValueArrayState& state, std::uint32_t object_address,
    std::uint32_t output_address, std::int32_t index,
    IIndexedSortedIteratorRuntime& runtime) {
    if (index >= 0 && index < 6) {
        const auto offset = static_cast<std::uint32_t>(index) * 16u;
        const auto address = runtime.Allocate(12u);
        if (address != 0u)
            runtime.StoreValid(address,
                               {kValidIteratorVtable,
                                state.slots[static_cast<std::size_t>(index)].begin,
                                object_address + offset});
        runtime.WriteOutput(output_address, address);
    } else {
        const auto address = runtime.Allocate(8u);
        if (address != 0u)
            runtime.StoreInvalid(address,
                                 {kInvalidIteratorVtable, 0xffffffffu});
        runtime.WriteOutput(output_address, address);
    }
    return output_address;
}

std::uint32_t IndexedFilteredValueRuntimeStub::GetGameDatabase() {
    return 0u;
}
std::uint32_t IndexedFilteredValueRuntimeStub::LookupRecord(
    std::uint32_t, std::uint32_t) {
    return 0u;
}
std::uint32_t IndexedFilteredValueRuntimeStub::ReadDword(std::uint32_t) {
    return 0u;
}
void IndexedFilteredValueRuntimeStub::WriteDword(
    std::uint32_t, std::uint32_t) {}
std::uint32_t IndexedFilteredValueRuntimeStub::Allocate(std::uint32_t) {
    return 0u;
}
void IndexedFilteredValueRuntimeStub::Free(std::uint32_t) {}

std::uint32_t IndexedFilteredValueCollector::Collect(
    const IndexedSortedValueArrayState& source,
    IndexedFilteredResultArrayState& result, std::uint32_t filter,
    IIndexedFilteredValueRuntime& runtime) {
    for (const auto& slot : source.slots) {
        auto cursor = slot.begin;
        while (cursor != slot.end) {
            const auto database = runtime.GetGameDatabase();
            const auto key = runtime.ReadDword(cursor);
            const auto record = runtime.LookupRecord(database + 12u, key);
            if (record != 0u && runtime.ReadDword(record + 8u) == filter)
                (void)DynamicDwordResultArray::Append(result, record, runtime);
            cursor += 4u;
        }
    }
    return 0u;
}

void FixedSortedValueArray::InsertUnique(
    FixedSortedValueArrayState& state, std::int32_t value,
    IIndexedSortedValueArrayRuntime& runtime) {
    IndexedSortedValueArrayState working{};
    working.slots[0] = state.values;
    (void)IndexedSortedValueArrays::InsertUnique(working, 0, value, runtime);
    state.values = working.slots[0];
}

bool FixedSortedValueArray::Contains(
    const FixedSortedValueArrayState& state, std::int32_t value,
    IIndexedSortedValueArrayRuntime& runtime) {
    IndexedSortedValueArrayState working{};
    working.slots[0] = state.values;
    return IndexedSortedValueArrays::Contains(working, 0, value, runtime);
}

std::uint32_t FixedSortedValueArray::GetIterator(
    const FixedSortedValueArrayState& state, std::uint32_t object_address,
    std::uint32_t output_address, IIndexedSortedIteratorRuntime& runtime) {
    IndexedSortedValueArrayState working{};
    working.slots[0] = state.values;
    return IndexedSortedValueArrays::GetIteratorByIndex(
        working, object_address + 0xa0u, output_address, 0, runtime);
}

std::uint32_t FixedSortedValueArray::Clear(
    FixedSortedValueArrayState& state) {
    const auto old_end = state.values.end;
    state.values.end = state.values.begin;
    return old_end;
}

std::uint32_t FixedSortedValueArray::ProcessValues(
    const FixedSortedValueArrayState& state, std::uint32_t context,
    ISortedValueProcessRuntime& runtime) {
    auto cursor = state.values.begin;
    while (cursor != state.values.end) {
        runtime.Process(context, runtime.ReadDword(cursor));
        cursor += 4u;
    }
    return state.values.end;
}

std::uint32_t FixedFilteredValueCollector::Collect(
    const FixedSortedValueArrayState& source,
    IndexedFilteredResultArrayState& result, std::uint32_t filter,
    IIndexedFilteredValueRuntime& runtime) {
    auto cursor = source.values.begin;
    while (cursor != source.values.end) {
        const auto database = runtime.GetGameDatabase();
        const auto key = runtime.ReadDword(cursor);
        const auto record = runtime.LookupRecord(database + 12u, key);
        if (record != 0u && runtime.ReadDword(record + 4u) == filter)
            (void)DynamicDwordResultArray::Append(result, record, runtime);
        cursor += 4u;
    }
    return source.values.end;
}

}  // namespace pal4
