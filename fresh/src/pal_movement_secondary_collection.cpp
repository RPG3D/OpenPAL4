#include "pal4/pal_movement_secondary_collection.h"

#include <algorithm>

namespace pal4 {

std::uint32_t PalMovementSecondaryCollectionRuntimeStub::ReadWord(
    std::uint32_t) { return 0u; }
void PalMovementSecondaryCollectionRuntimeStub::WriteWord(
    std::uint32_t, std::uint32_t) {}
std::uint32_t PalMovementSecondaryCollectionRuntimeStub::GetClump(
    std::uint32_t) { return 0u; }
std::uint32_t PalMovementSecondaryCollectionRuntimeStub::Allocate(
    std::uint32_t) { return 0u; }
void PalMovementSecondaryCollectionRuntimeStub::Free(std::uint32_t) {}
void PalMovementSecondaryCollectionRuntimeStub::Leave(std::uint32_t) {}

std::uint32_t PalMovementSecondaryChild::AddClump(
    PalMovementSecondaryChildState& state, std::uint32_t element,
    IPalMovementSecondaryCollectionRuntime& runtime) {
    for (auto cursor = state.words[28]; cursor != state.words[29];
         cursor += 4u) {
        const auto existing = runtime.ReadWord(cursor);
        const auto existing_clump = runtime.GetClump(existing);
        const auto candidate_clump = runtime.GetClump(element);
        if (existing_clump == candidate_clump)
            return 0u;
    }

    const auto old_begin = state.words[28];
    const auto old_end = state.words[29];
    if (static_cast<std::int32_t>(state.words[30] - old_end) >= 4) {
        runtime.WriteWord(old_end, element);
        state.words[29] = old_end + 4u;
        return 1u;
    }

    const auto old_size = old_begin == 0u ? 0u :
        (old_end - old_begin) / 4u;
    const auto growth = std::max(old_size, 1u);
    const auto new_count = old_size + growth;
    const auto new_begin = runtime.Allocate(new_count * 4u);
    auto destination = new_begin;
    for (auto source = old_begin; source != old_end; source += 4u) {
        if (destination != 0u)
            runtime.WriteWord(destination, runtime.ReadWord(source));
        destination += 4u;
    }
    if (destination != 0u) runtime.WriteWord(destination, element);
    runtime.Free(old_begin);
    state.words[30] = new_begin + new_count * 4u;
    state.words[28] = new_begin;
    state.words[29] = new_begin + (old_size + 1u) * 4u;
    return 1u;
}

std::uint32_t PalMovementSecondaryChild::RemoveClump(
    PalMovementSecondaryChildState& state, std::uint32_t element,
    IPalMovementSecondaryCollectionRuntime& runtime) {
    for (auto cursor = state.words[28]; cursor != state.words[29];
         cursor += 4u) {
        const auto existing = runtime.ReadWord(cursor);
        const auto existing_clump = runtime.GetClump(existing);
        const auto candidate_clump = runtime.GetClump(element);
        if (existing_clump != candidate_clump)
            continue;
        for (auto source = cursor + 4u; source != state.words[29];
             source += 4u)
            runtime.WriteWord(source - 4u, runtime.ReadWord(source));
        state.words[29] -= 4u;
        state.words[31] = 0u;
        return 1u;
    }
    return 0u;
}

std::uint32_t PalMovementSecondaryChild::GetActiveClump(
    const PalMovementSecondaryChildState& state,
    IPalMovementSecondaryCollectionRuntime& runtime) {
    const auto active = state.words[31];
    return active == 0u ? 0u : runtime.GetClump(active);
}

std::uint32_t PalMovementSecondaryChild::ClearActiveClump(
    PalMovementSecondaryChildState& state,
    IPalMovementSecondaryCollectionRuntime& runtime) {
    for (auto cursor = state.words[28]; cursor != state.words[29];
         cursor += 4u) {
        const auto active = state.words[31];
        if (active == runtime.ReadWord(cursor)) runtime.Leave(active);
    }
    const auto end = state.words[29];
    state.words[31] = 0u;
    return end;
}

}  // namespace pal4
