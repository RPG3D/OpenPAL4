#include "pal4/indexed_key_value_collection.h"

namespace pal4 {
namespace {

std::uint32_t LowerBound(const KeyValueArraySlot& slot, std::int32_t key,
                         IIndexedKeyValueCollectionRuntime& runtime) {
    auto position = slot.begin;
    auto count = static_cast<std::int32_t>(slot.end - position) >> 3;
    while (count > 0) {
        const auto midpoint = static_cast<std::uint32_t>(count / 2);
        const auto candidate = static_cast<std::int32_t>(
            runtime.ReadDword(position + midpoint * 8u));
        if (candidate >= key) {
            count /= 2;
        } else {
            position += (midpoint + 1u) * 8u;
            count -= static_cast<std::int32_t>(midpoint + 1u);
        }
    }
    return position;
}

}  // namespace

std::uint32_t IndexedKeyValueCollectionRuntimeStub::ReadDword(std::uint32_t) {
    return 0u;
}
void IndexedKeyValueCollectionRuntimeStub::WriteDword(
    std::uint32_t, std::uint32_t) {}
std::uint32_t IndexedKeyValueCollectionRuntimeStub::Allocate(std::uint32_t) {
    return 0u;
}
void IndexedKeyValueCollectionRuntimeStub::Free(std::uint32_t) {}

void IndexedKeyValueCollection::AddDelta(
    IndexedKeyValueCollectionState& state, std::int32_t index,
    std::int32_t key, std::uint32_t delta,
    IIndexedKeyValueCollectionRuntime& runtime) {
    if (index < 0 || index >= 6)
        return;

    auto& slot = state.slots[static_cast<std::size_t>(index)];
    const auto position = LowerBound(slot, key, runtime);

    if (position != slot.end &&
        runtime.ReadDword(position) == static_cast<std::uint32_t>(key)) {
        const auto old_value = runtime.ReadDword(position + 4u);
        runtime.WriteDword(position + 4u, old_value + delta);
        return;
    }

    if (slot.capacity_end != slot.end) {
        for (auto source = slot.end; source > position; source -= 8u) {
            runtime.WriteDword(source, runtime.ReadDword(source - 8u));
            runtime.WriteDword(source + 4u,
                               runtime.ReadDword(source - 4u));
        }
        runtime.WriteDword(position, static_cast<std::uint32_t>(key));
        runtime.WriteDword(position + 4u, delta);
        slot.end += 8u;
        return;
    }

    const auto old_begin = slot.begin;
    const auto old_size = old_begin == 0u ? 0u :
                          (slot.end - old_begin) / 8u;
    const auto prefix = old_begin == 0u ? 0u :
                        (position - old_begin) / 8u;
    const auto new_capacity = old_size +
                              (old_size > 1u ? old_size : 1u);
    const auto replacement = runtime.Allocate(new_capacity * 8u);
    for (std::uint32_t i = 0; i < prefix; ++i) {
        runtime.WriteDword(replacement + i * 8u,
                           runtime.ReadDword(old_begin + i * 8u));
        runtime.WriteDword(replacement + i * 8u + 4u,
                           runtime.ReadDword(old_begin + i * 8u + 4u));
    }
    runtime.WriteDword(replacement + prefix * 8u,
                       static_cast<std::uint32_t>(key));
    runtime.WriteDword(replacement + prefix * 8u + 4u, delta);
    for (auto i = prefix; i < old_size; ++i) {
        runtime.WriteDword(replacement + (i + 1u) * 8u,
                           runtime.ReadDword(old_begin + i * 8u));
        runtime.WriteDword(replacement + (i + 1u) * 8u + 4u,
                           runtime.ReadDword(old_begin + i * 8u + 4u));
    }
    runtime.Free(old_begin);
    slot.begin = replacement;
    slot.end = replacement + (old_size + 1u) * 8u;
    slot.capacity_end = replacement + new_capacity * 8u;
}

std::uint32_t IndexedKeyValueCollection::GetValue(
    const IndexedKeyValueCollectionState& state, std::int32_t index,
    std::int32_t key, IIndexedKeyValueCollectionRuntime& runtime) {
    if (index < 0 || index >= 6)
        return 0u;
    const auto& slot = state.slots[static_cast<std::size_t>(index)];
    const auto position = LowerBound(slot, key, runtime);
    if (position == slot.end ||
        runtime.ReadDword(position) != static_cast<std::uint32_t>(key))
        return 0u;
    return runtime.ReadDword(position + 4u);
}

std::uint32_t IndexedKeyValueCollection::TotalValue(
    const IndexedKeyValueCollectionState& state, std::int32_t index,
    IIndexedKeyValueCollectionRuntime& runtime) {
    if (index < 0 || index >= 6)
        return 0u;
    const auto& slot = state.slots[static_cast<std::size_t>(index)];
    if (slot.begin == 0u)
        return 0u;
    std::uint32_t total = 0u;
    std::uint32_t cursor = 0u;
    while (cursor < (slot.end - slot.begin) / 8u) {
        total += runtime.ReadDword(slot.begin + cursor * 8u + 4u);
        ++cursor;
    }
    return total;
}

std::uint32_t IndexedKeyValueCollection::RemainingValue(
    const IndexedKeyValueCollectionState& state, std::uint32_t input,
    IIndexedKeyValueCollectionRuntime& runtime) {
    std::uint32_t used = 0u;
    for (std::int32_t index = 0; index < 5; ++index)
        used += TotalValue(state, index, runtime);
    return input - used;
}

}  // namespace pal4
