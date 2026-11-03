#pragma once

#include "pal4/indexed_key_value_collection.h"
#include "pal4/indexed_sorted_value_arrays.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct IndexedCollectionResetState {
    std::array<SortedValueArraySlot, 5> values{};
    std::array<KeyValueArraySlot, 6> key_values{};
};
static_assert(offsetof(IndexedCollectionResetState, key_values) == 0x50u);
static_assert(sizeof(IndexedCollectionResetState) == 0xb0u);

class IndexedCollectionReset final {
public:
    [[nodiscard]] static std::uint32_t ClearFirstFive(
        IndexedCollectionResetState& state);
    [[nodiscard]] static bool HasAnyValues(
        const IndexedCollectionResetState& state);
    [[nodiscard]] static std::uint32_t ProcessValues(
        const IndexedCollectionResetState& state, std::uint32_t context,
        ISortedValueProcessRuntime& runtime);
};

}  // namespace pal4
