#include "pal4/collection_lifecycle.h"

namespace pal4 {

CollectionState& CollectionLifecycle::Construct(
    CollectionState& state, std::uint8_t initial_byte,
    std::uint8_t nested_byte,
    ICollectionLifecycleRuntime& runtime) {
    state.initial_byte = initial_byte;
    state.first = nullptr;
    state.last = nullptr;
    state.limit = nullptr;
    state.base.references = 1u;
    state.base.vtable = CollectionState::kVtable;
    InitializeNested(state, nested_byte, runtime);

    // The freshly cleared header always takes the eight-word allocation path.
    auto* storage = runtime.AllocateWords(kInitialCapacity);
    runtime.FreeWords(state.first);
    state.first = storage;
    state.last = storage;
    state.limit = storage ? storage + kInitialCapacity : nullptr;
    return state;
}

void CollectionLifecycle::InitializeNested(
    CollectionState& state, std::uint8_t nested_byte,
    ICollectionLifecycleRuntime& runtime) {
    runtime.ConstructGameWorld(state);
    auto& world = state.world;
    world.byte_c4 = nested_byte;
    world.field_c8 = 0u;
    world.field_cc = 0u;
    world.field_d0 = 0u;
    world.byte_d4 = nested_byte;
    world.field_d8 = 0u;
    world.field_dc = 0u;
    world.field_e0 = 0u;
    world.vtable = CollectionWorldState::kVtable;
}

void CollectionLifecycle::ConstructBase(CollectionBaseState& base) {
    base.references = 1u;
    base.vtable = CollectionState::kBaseVtable;
}

CollectionBaseState& CollectionLifecycle::DeleteBase(
    CollectionBaseState& base, std::uint32_t flags,
    ICollectionLifecycleRuntime& runtime) {
    base.vtable = CollectionState::kBaseVtable;
    if ((flags & 1u) != 0u) runtime.FreeBase(base);
    return base;
}

const char* CollectionLifecycle::IdGeneratorName() noexcept {
    return "palIdGenerator";
}

int CollectionLifecycle::Clear(CollectionState& state,
                               std::uint32_t receiver,
                               ICollectionLifecycleRuntime& runtime) {
    return runtime.Clear(receiver, state, 4u, 4u);
}

int CollectionLifecycle::ClearAndFree(CollectionState& state,
                                      std::uint32_t receiver,
                                      ICollectionLifecycleRuntime& runtime) {
    return runtime.ClearAndFree(receiver, state, 4u, 4u);
}

int CollectionLifecycle::CleanupGameObjects(
    CollectionState& state, ICollectionLifecycleRuntime& runtime) {
    const auto clump = runtime.WorldClump(state);
    if (clump != 0u) runtime.RemoveWorldClump(clump);
    runtime.CleanupAtomicGeometry(state, 0xdcu);
    const auto game = runtime.GameInstance();
    return runtime.CleanupWorld(state, game + 0x1acu);
}

void CollectionLifecycle::CleanupElements(
    CollectionState& state, ICollectionLifecycleRuntime& runtime) {
    std::size_t index = 0u;
    while (state.first && state.last &&
           index < static_cast<std::size_t>(state.last - state.first)) {
        const auto object = state.first[index++];
        if (object != 0u) runtime.DeleteElement(object, 1u);
    }
    state.last = state.first;
}

void CollectionLifecycle::DestroyNested(
    CollectionState& state, ICollectionLifecycleRuntime& runtime) {
    auto& world = state.world;
    world.vtable = CollectionWorldState::kVtable;
    runtime.CleanupAtomicGeometry(state, 0xdcu);
    if (world.field_d8 != 0u) {
        const auto prefix = world.field_d8 - 1u;
        const auto references = runtime.ReadPrefixByte(prefix);
        if (references == 0u || references == 0xffu) {
            runtime.FreeWorldBuffer(prefix);
        } else {
            runtime.WritePrefixByte(prefix,
                                    static_cast<std::uint8_t>(references - 1u));
        }
    }
    world.field_d8 = 0u;
    world.field_dc = 0u;
    world.field_e0 = 0u;
    runtime.FreeWorldBuffer(world.field_c8);
    world.field_c8 = 0u;
    world.field_cc = 0u;
    world.field_d0 = 0u;
    runtime.DestroyGameWorld(state);
}

void CollectionLifecycle::Destroy(CollectionState& state,
                                  ICollectionLifecycleRuntime& runtime) {
    CleanupElements(state, runtime);
    (void)CleanupGameObjects(state, runtime);
    DestroyNested(state, runtime);
    ConstructBase(state.base);
    runtime.FreeWords(state.first);
    state.first = nullptr;
    state.last = nullptr;
    state.limit = nullptr;
}

}  // namespace pal4
