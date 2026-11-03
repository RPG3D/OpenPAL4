#include "pal4/indexed_collection_reset.h"

namespace pal4 {

std::uint32_t IndexedCollectionReset::ClearFirstFive(
    IndexedCollectionResetState& state) {
    std::uint32_t last_end = 0u;
    for (std::size_t i = 0; i < 5u; ++i) {
        state.values[i].end = state.values[i].begin;
        last_end = state.key_values[i].end;
        state.key_values[i].end = state.key_values[i].begin;
    }
    return last_end;
}

bool IndexedCollectionReset::HasAnyValues(
    const IndexedCollectionResetState& state) {
    for (const auto& slot : state.values) {
        if (slot.begin != 0u && slot.begin != slot.end)
            return true;
    }
    return false;
}

std::uint32_t IndexedCollectionReset::ProcessValues(
    const IndexedCollectionResetState& state, std::uint32_t context,
    ISortedValueProcessRuntime& runtime) {
    for (const auto& slot : state.values) {
        auto cursor = slot.begin;
        while (cursor != slot.end) {
            runtime.Process(context, runtime.ReadDword(cursor));
            cursor += 4u;
        }
    }
    return 0u;
}

}  // namespace pal4
