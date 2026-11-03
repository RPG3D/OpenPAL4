#include "pal4/pal_behavior_class_one.h"

namespace pal4 {

void PalBehaviorClassOneRuntimeStub::ConstructBase(
    PalBehaviorClassOneState&) {}
void PalBehaviorClassOneRuntimeStub::DestructBase(
    PalBehaviorClassOneState&) {}
std::uint8_t PalBehaviorClassOneRuntimeStub::ReadByte(
    std::uint32_t) { return 0u; }
void PalBehaviorClassOneRuntimeStub::ReleaseOwned(
    std::uint32_t, std::uint32_t) {}
void PalBehaviorClassOneRuntimeStub::Free(std::uint32_t) {}
std::uint32_t PalBehaviorClassOneRuntimeStub::Bind(
    std::uint32_t, std::uint32_t&) { return 0u; }
std::uint32_t PalBehaviorClassOneRuntimeStub::Allocate(
    std::uint32_t) { return 0u; }
std::uint32_t PalBehaviorClassOneRuntimeStub::InitializeVectorObject(
    std::uint32_t address) { return address; }
void PalBehaviorClassOneRuntimeStub::OnCreated(
    std::uint32_t, std::uint32_t) {}
PalBehaviorClassOneState& PalBehaviorClassOneRuntimeStub::Access(
    std::uint32_t) { return state_; }

PalBehaviorClassOneState& PalBehaviorClassOne::Construct(
    PalBehaviorClassOneState& state,
    IPalBehaviorClassOneRuntime& runtime) {
    runtime.ConstructBase(state);
    state.words[0] = kVtable;
    state.words[15] = 0u;
    state.words[16] = 0u;
    return state;
}

std::uint32_t PalBehaviorClassOne::Delete(
    PalBehaviorClassOneState& state, std::uint32_t address,
    std::uint32_t flags, IPalBehaviorClassOneRuntime& runtime) {
    Destruct(state, runtime);
    if ((flags & 1u) != 0u) runtime.Free(address);
    return address;
}

void PalBehaviorClassOne::Destruct(
    PalBehaviorClassOneState& state,
    IPalBehaviorClassOneRuntime& runtime) {
    state.words[0] = kVtable;
    const auto object = state.words[16];
    if (object != 0u && runtime.ReadByte(object + 0xb8u) != 0u)
        runtime.ReleaseOwned(state.words[15], object);
    state.words[15] = 0u;
    state.words[16] = 0u;
    state.words[0] = kBaseVtable;
    runtime.DestructBase(state);
}

std::uint32_t PalBehaviorClassOne::BindOwner(
    PalBehaviorClassOneState& state, std::uint32_t binder,
    IPalBehaviorClassOneRuntime& runtime) {
    std::uint32_t initial_value = 1u;
    const auto result = runtime.Bind(binder, initial_value);
    state.words[15] = result == 0u ? 0u : result - 4u;
    return state.words[15];
}

std::uint32_t PalBehaviorClassOne::CreateOwned(
    PalBehaviorClassOneState& state,
    IPalBehaviorClassOneRuntime& runtime) {
    const auto owner = state.words[15];
    if (owner == 0u) return 0u;
    const auto allocated = runtime.Allocate(0xe0u);
    std::uint32_t object = 0u;
    if (allocated != 0u) {
        try {
            object = runtime.InitializeVectorObject(allocated);
        } catch (...) {
            runtime.Free(allocated);
            throw;
        }
    }
    state.words[16] = object;
    runtime.OnCreated(owner, object);
    return 1u;
}

std::uint32_t PalBehaviorClassOne::Create(
    IPalBehaviorClassOneRuntime& runtime) {
    const auto allocated = runtime.Allocate(0x44u);
    if (allocated == 0u) return 0u;
    try {
        static_cast<void>(Construct(runtime.Access(allocated), runtime));
    } catch (...) {
        runtime.Free(allocated);
        throw;
    }
    return allocated;
}

}  // namespace pal4
