#include "pal4/pal_movement_secondary_child.h"

namespace pal4 {

void PalMovementSecondaryChildRuntimeStub::InitializeConfig(
    PalMovementSecondaryChildState&) {}
void PalMovementSecondaryChildRuntimeStub::Free(std::uint32_t) {}
std::uint32_t PalMovementSecondaryChildRuntimeStub::CleanupConfig(
    PalMovementSecondaryChildState&) { return 0u; }
void PalMovementSecondaryChildRuntimeStub::CleanupAlternate(
    PalMovementSecondaryChildState&) {}
std::uint32_t PalMovementSecondaryChildRuntimeStub::ReadElement(
    std::uint32_t) { return 0u; }
std::uint32_t PalMovementSecondaryChildRuntimeStub::GetClump(
    std::uint32_t) { return 0u; }
void PalMovementSecondaryChildRuntimeStub::ResetAtomicCallbacks(
    std::uint32_t) {}

PalMovementSecondaryChildState& PalMovementSecondaryChild::Construct(
    PalMovementSecondaryChildState& state,
    std::uint8_t observed_stack_byte,
    IPalMovementSecondaryChildRuntime& runtime) {
    runtime.InitializeConfig(state);
    state.words[27] = (state.words[27] & 0xffffff00u) |
                      static_cast<std::uint32_t>(observed_stack_byte);
    state.words[28] = 0u;
    state.words[29] = 0u;
    state.words[30] = 0u;
    state.words[31] = 0u;
    state.words[33] = 0u;
    state.words[0] = kVtable;
    state.words[32] = 0xffffffffu;
    return state;
}

std::uint32_t PalMovementSecondaryChild::ReadField80(
    const PalMovementSecondaryChildState& state) {
    return state.words[32];
}

std::uint32_t PalMovementSecondaryChild::ReadField84(
    const PalMovementSecondaryChildState& state) {
    return state.words[33];
}

std::uint32_t PalMovementSecondaryChild::Delete(
    PalMovementSecondaryChildState& state, std::uint32_t address,
    std::uint32_t flags, IPalMovementSecondaryChildRuntime& runtime) {
    static_cast<void>(Destruct(state, runtime));
    if ((flags & 1u) != 0u) runtime.Free(address);
    return address;
}

std::uint32_t PalMovementSecondaryChild::Destruct(
    PalMovementSecondaryChildState& state,
    IPalMovementSecondaryChildRuntime& runtime) {
    state.words[0] = kVtable;
    state.words[31] = 0u;
    runtime.Free(state.words[28]);
    state.words[28] = 0u;
    state.words[29] = 0u;
    state.words[30] = 0u;
    state.words[0] = kBaseVtable;
    return runtime.CleanupConfig(state);
}

std::uint32_t PalMovementSecondaryChild::ClearClumpCallbacks(
    PalMovementSecondaryChildState& state,
    IPalMovementSecondaryChildRuntime& runtime) {
    runtime.CleanupAlternate(state);
    for (auto cursor = state.words[28]; cursor != state.words[29];
         cursor += 4u) {
        const auto element = runtime.ReadElement(cursor);
        runtime.ResetAtomicCallbacks(runtime.GetClump(element));
    }
    const auto previous_end = state.words[29];
    state.words[29] = state.words[28];
    state.words[31] = 0u;
    return previous_end;
}

}  // namespace pal4
