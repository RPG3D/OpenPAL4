#include "pal4/script_unique_collection.h"

#include <new>

namespace pal4 {

std::uint32_t ScriptUniqueCollection::AddUnique(
    ScriptUniqueCollectionState& state, std::uint32_t value) {
    const auto begin = state.At(4u);
    const auto end = state.At(8u);
    for (auto current = begin; current != end; current += 4u) {
        if (runtime_.ReadWord(current) == value) return end;
    }

    if (state.At(12u) != end) {
        runtime_.WriteWord(end, value);
        state.At(8u) = end + 4u;
        return end;
    }

    const auto size = begin == 0u ? 0u : (end - begin) / 4u;
    const auto growth = size > 1u ? size : 1u;
    const auto capacity = size + growth;
    const auto replacement = runtime_.Allocate(capacity * 4u);
    if (replacement == 0u) throw std::bad_alloc();
    for (std::uint32_t index = 0u; index < size; ++index) {
        runtime_.WriteWord(replacement + 4u * index,
                           runtime_.ReadWord(begin + 4u * index));
    }
    runtime_.WriteWord(replacement + 4u * size, value);
    runtime_.Free(begin);
    state.At(12u) = replacement + 4u * capacity;
    state.At(4u) = replacement;
    state.At(8u) = replacement + 4u * (size + 1u);
    return size;
}

void ScriptUniqueCollection::Remove(
    ScriptUniqueCollectionState& state, std::uint32_t value) {
    const auto end = state.At(8u);
    for (auto current = state.At(4u); current != end; current += 4u) {
        if (runtime_.ReadWord(current) != value) continue;
        for (auto next = current + 4u; next != end; next += 4u)
            runtime_.WriteWord(next - 4u, runtime_.ReadWord(next));
        state.At(8u) = end - 4u;
        return;
    }
}

}  // namespace pal4
