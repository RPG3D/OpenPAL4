#include "pal4/pal_movement_primary_child.h"

namespace pal4 {

void PalMovementPrimaryChildRuntimeStub::InitializeConfig(
    PalMovementPrimaryChildState&) {}
std::uint32_t PalMovementPrimaryChildRuntimeStub::CleanupConfig2(
    PalMovementPrimaryChildState&) { return 0u; }
std::uint32_t PalMovementPrimaryChildRuntimeStub::CleanupConfig3(
    PalMovementPrimaryChildState&) { return 0u; }
void PalMovementPrimaryChildRuntimeStub::Free(std::uint32_t) {}

PalMovementPrimaryChildState& PalMovementPrimaryChild::Construct(
    PalMovementPrimaryChildState& state,
    IPalMovementPrimaryChildRuntime& runtime) {
    runtime.InitializeConfig(state);
    state.words[0] = kVtable;
    state.words[27] = 0xffffffffu;
    state.words[28] = 0u;
    return state;
}

std::uint32_t PalMovementPrimaryChild::AlwaysReady(
    const PalMovementPrimaryChildState&, std::uint32_t,
    std::uint32_t, std::uint32_t) {
    return 1u;
}

std::uint32_t PalMovementPrimaryChild::ReadField34(
    const PalMovementPrimaryChildState& state) {
    return state.words[13];
}

std::uint32_t PalMovementPrimaryChild::ReadField6C(
    const PalMovementPrimaryChildState& state) {
    return state.words[27];
}

std::uint32_t PalMovementPrimaryChild::ReadField70(
    const PalMovementPrimaryChildState& state) {
    return state.words[28];
}

std::uint32_t PalMovementPrimaryChild::Delete(
    PalMovementPrimaryChildState& state, std::uint32_t address,
    std::uint32_t flags, IPalMovementPrimaryChildRuntime& runtime) {
    static_cast<void>(Destruct(state, runtime));
    if ((flags & 1u) != 0u) runtime.Free(address);
    return address;
}

std::uint32_t PalMovementPrimaryChild::Destruct(
    PalMovementPrimaryChildState& state,
    IPalMovementPrimaryChildRuntime& runtime) {
    state.words[0] = kBaseVtable;
    return runtime.CleanupConfig2(state);
}

std::uint32_t PalMovementPrimaryChild::CleanupAlternate(
    PalMovementPrimaryChildState& state,
    IPalMovementPrimaryChildRuntime& runtime) {
    return runtime.CleanupConfig3(state);
}

std::uint32_t PalMovementBaseChild::Delete(
    PalMovementPrimaryChildState& state, std::uint32_t address,
    std::uint32_t flags, IPalMovementPrimaryChildRuntime& runtime) {
    static_cast<void>(Destruct(state, runtime));
    if ((flags & 1u) != 0u) runtime.Free(address);
    return address;
}

std::uint32_t PalMovementBaseChild::Destruct(
    PalMovementPrimaryChildState& state,
    IPalMovementPrimaryChildRuntime& runtime) {
    state.words[0] = PalMovementPrimaryChild::kBaseVtable;
    return runtime.CleanupConfig2(state);
}

}  // namespace pal4
