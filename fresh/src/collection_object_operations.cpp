#include "pal4/collection_object_operations.h"

namespace pal4 {

int CollectionObjectOperations::AddElement(
    CollectionState& state, std::uint32_t object, std::uint32_t name,
    ICollectionObjectRuntime& runtime) {
    const auto camera = runtime.ActiveCameraWord();
    const auto name_text = runtime.NameText(name);
    return CollectionWorldInitializer::AddElementWithDetails(
        state, object, name_text, camera, runtime.WorldRuntime());
}

int CollectionObjectOperations::RenderWorldThunk(
    CollectionState& state, int argument, ICollectionObjectRuntime& runtime) {
    return runtime.RenderWorld(state, argument);
}

int CollectionObjectOperations::GetGameObjectCount(
    CollectionState& state, int argument, ICollectionObjectRuntime& runtime) {
    return RenderWorldThunk(state, argument, runtime);
}

int CollectionObjectOperations::GetGameObject(
    CollectionState& state, int, ICollectionObjectRuntime& runtime) {
    return runtime.SortAtomicGeometries(state, 0xdcu);
}

std::uint32_t CollectionObjectOperations::CreateGameObject(
    CollectionState& state, std::uint32_t prefix, std::uint32_t name,
    std::uint32_t variant, ICollectionObjectRuntime& runtime) {
    const auto storage = runtime.AllocateGameObject(0x184u);
    std::uint32_t object = storage ? runtime.ConstructComponent(storage) : 0u;
    const auto id = state.base.references++;
    runtime.LoadComponent(object, id, prefix, name, variant, 0u);
    const auto world = runtime.WorldMember(state);
    runtime.AddToWorld(object, world);
    runtime.ProcessEvent(state, state.last, 1u, object);
    const auto clump = runtime.ObjectClump(object);
    if (clump != 0u) {
        runtime.ProcessClumpAtomics(clump, state, 0xdcu);
        runtime.HideHookAtomics(clump);
    }
    return object;
}

std::uint32_t CollectionObjectOperations::FindGameObjectById(
    const CollectionState& state, std::uint32_t id,
    ICollectionObjectRuntime& runtime) {
    if (!state.first || !state.last) return 0u;
    for (auto* cursor = state.first; cursor != state.last; ++cursor) {
        if (runtime.ObjectId(*cursor, 0x170u) == id) return *cursor;
    }
    return 0u;
}

std::uint32_t* CollectionObjectOperations::RemoveGameObjectById(
    CollectionState& state, std::uint32_t id,
    ICollectionObjectRuntime& runtime) {
    auto* old_end = state.last;
    if (!state.first || !state.last) return old_end;
    auto* cursor = state.first;
    while (cursor != state.last && runtime.ObjectId(*cursor, 0x170u) != id) {
        ++cursor;
    }
    if (cursor == state.last) return old_end;

    const auto object = *cursor;
    runtime.RemoveFromWorld(object);
    runtime.CleanupComponent(object);
    if (object != 0u) runtime.DeleteComponent(object, 1u);
    for (auto* next = cursor + 1; next != state.last; ++next, ++cursor) {
        *cursor = *next;
    }
    --state.last;
    return old_end;
}

}  // namespace pal4
