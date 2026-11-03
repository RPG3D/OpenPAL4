#include "pal4/pal_behavior_tasks.h"

namespace pal4 {

void PalBehaviorTaskRuntimeStub::ConstructBase(PalBehaviorTaskState&) {}
void PalBehaviorTaskRuntimeStub::DestructBase(PalBehaviorTaskState&) {}
std::uint8_t PalBehaviorTaskRuntimeStub::ReadByte(std::uint32_t) { return 0u; }
void PalBehaviorTaskRuntimeStub::WriteByte(std::uint32_t, std::uint8_t) {}
void PalBehaviorTaskRuntimeStub::ReleaseOwned(std::uint32_t, std::uint32_t) {}
void PalBehaviorTaskRuntimeStub::Free(std::uint32_t) {}
std::uint32_t PalBehaviorTaskRuntimeStub::Bind(std::uint32_t,
                                                std::uint32_t&) { return 0u; }
std::uint32_t PalBehaviorTaskRuntimeStub::Allocate(std::uint32_t) { return 0u; }
std::uint32_t PalBehaviorTaskRuntimeStub::ConstructVectorObject(
    std::uint32_t address) { return address; }
std::uint32_t PalBehaviorTaskRuntimeStub::ConstructExtendedObject(
    std::uint32_t address) { return address; }
std::uint32_t PalBehaviorTaskRuntimeStub::ConstructSmallObject(
    std::uint32_t address) { return address; }
std::uint32_t PalBehaviorTaskRuntimeStub::ConstructRandomObject(
    std::uint32_t address) { return address; }
void PalBehaviorTaskRuntimeStub::OnCreated(std::uint32_t, std::uint32_t) {}
void PalBehaviorTaskRuntimeStub::UpdateOwned(std::uint32_t, std::uint32_t) {}
std::uint32_t PalBehaviorTaskRuntimeStub::IsOwnedComplete(
    std::uint32_t, std::uint32_t) { return 0u; }
PalBehaviorTaskState& PalBehaviorTaskRuntimeStub::Access(std::uint32_t) {
    return state_;
}
bool PalBehaviorTaskRuntimeStub::IsDurationComplete(PalBehaviorTaskState&) {
    return false;
}

void PalBehaviorTaskBase::Destruct(PalBehaviorTaskState& state,
                                   IPalBehaviorTaskRuntime& runtime) {
    state.words[0] = kVtable;
    runtime.DestructBase(state);
}

bool PalBehaviorTaskBase::Finish(PalBehaviorTaskState& state,
                                  IPalBehaviorTaskExecutionRuntime& runtime) {
    const auto object = state.words[16];
    if (object == 0u || state.words[15] == 0u) return false;
    runtime.ReleaseOwned(state.words[15], object);
    state.words[16] = 0u;
    return true;
}

bool PalBehaviorTaskBase::Run(PalBehaviorTaskState& state,
                               IPalBehaviorTaskExecutionRuntime& runtime) {
    const auto object = state.words[16];
    const auto owner = state.words[15];
    if (object == 0u || owner == 0u) return false;
    if (runtime.ReadByte(object + 0xb8u) != 0u)
        runtime.UpdateOwned(owner, object);
    return true;
}

bool PalBehaviorTaskBase::IsDurationComplete(
    PalBehaviorTaskState& state, IPalBehaviorTaskRuntime& runtime) {
    return runtime.IsDurationComplete(state);
}

template <PalBehaviorKind Kind>
PalBehaviorTaskState& PalBehaviorClass<Kind>::Construct(
    PalBehaviorTaskState& state, IPalBehaviorTaskRuntime& runtime) {
    runtime.ConstructBase(state);
    state.words[0] = kVtable;
    state.words[15] = 0u;
    state.words[16] = 0u;
    return state;
}

template <PalBehaviorKind Kind>
std::uint32_t PalBehaviorClass<Kind>::Delete(
    PalBehaviorTaskState& state, std::uint32_t address,
    std::uint32_t flags, IPalBehaviorTaskRuntime& runtime) {
    Destruct(state, runtime);
    if ((flags & 1u) != 0u) runtime.Free(address);
    return address;
}

template <PalBehaviorKind Kind>
void PalBehaviorClass<Kind>::Destruct(PalBehaviorTaskState& state,
                                      IPalBehaviorTaskRuntime& runtime) {
    state.words[0] = kVtable;
    const auto object = state.words[16];
    if (object != 0u && runtime.ReadByte(object + 0xb8u) != 0u)
        runtime.ReleaseOwned(state.words[15], object);
    state.words[15] = 0u;
    state.words[16] = 0u;
    PalBehaviorTaskBase::Destruct(state, runtime);
}

template <PalBehaviorKind Kind>
std::uint32_t PalBehaviorClass<Kind>::BindOwner(
    PalBehaviorTaskState& state, std::uint32_t binder,
    IPalBehaviorTaskRuntime& runtime) {
    std::uint32_t type_id = kTypeId;
    const auto result = runtime.Bind(binder, type_id);
    state.words[15] = result == 0u ? 0u : result - 4u;
    return state.words[15];
}

template <PalBehaviorKind Kind>
std::uint32_t PalBehaviorClass<Kind>::CreateOwned(
    PalBehaviorTaskState& state, IPalBehaviorTaskRuntime& runtime) {
    const auto owner = state.words[15];
    if (owner == 0u) return 0u;
    const auto allocated = runtime.Allocate(kOwnedBytes);
    std::uint32_t object = 0u;
    try {
        if (allocated != 0u) {
            if constexpr (Kind == PalBehaviorKind::ClassZero)
                object = runtime.ConstructVectorObject(allocated);
            else if constexpr (Kind == PalBehaviorKind::ClassFour)
                object = runtime.ConstructExtendedObject(allocated);
            else if constexpr (Kind == PalBehaviorKind::ClassTwo)
                object = runtime.ConstructSmallObject(allocated);
            else
                object = runtime.ConstructRandomObject(allocated);
        }
        state.words[16] = object;
        if constexpr (Kind == PalBehaviorKind::ClassTwo)
            runtime.WriteByte(object + 0xb8u, 0u);
    } catch (...) {
        if (allocated != 0u || Kind == PalBehaviorKind::ClassTwo)
            runtime.Free(allocated);
        throw;
    }
    runtime.OnCreated(owner, object);
    return 1u;
}

template <PalBehaviorKind Kind>
std::uint32_t PalBehaviorClass<Kind>::Create(
    IPalBehaviorTaskRuntime& runtime) {
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

template <PalBehaviorKind Kind>
bool PalBehaviorClass<Kind>::IsComplete(
    PalBehaviorTaskState& state, IPalBehaviorTaskRuntime& runtime) {
    if constexpr (Kind == PalBehaviorKind::ClassTwo) {
        if (state.words[16] == 0u || state.words[15] == 0u) return false;
        return static_cast<std::uint8_t>(runtime.IsOwnedComplete(
                   state.words[15], state.words[16])) != 0u;
    } else if constexpr (Kind == PalBehaviorKind::ClassThree) {
        return state.words[16] != 0u && state.words[15] != 0u &&
               PalBehaviorTaskBase::IsDurationComplete(state, runtime);
    } else {
        return PalBehaviorTaskBase::IsDurationComplete(state, runtime);
    }
}

template class PalBehaviorClass<PalBehaviorKind::ClassZero>;
template class PalBehaviorClass<PalBehaviorKind::ClassTwo>;
template class PalBehaviorClass<PalBehaviorKind::ClassThree>;
template class PalBehaviorClass<PalBehaviorKind::ClassFour>;

}  // namespace pal4
