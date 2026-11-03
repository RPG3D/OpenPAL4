#include "pal4/collection_object_operations.h"

namespace pal4 {

void CollectionObjectOperations::RotateGameObjects(
    CollectionState& state, double elapsed, ICollectionUpdateRuntime& runtime) {
    (void)UpdateRenderParameters(state, elapsed, runtime);
    std::size_t index = 0u;
    while (state.first && state.last &&
           index < static_cast<std::size_t>(state.last - state.first)) {
        runtime.RotateComponent(state.first[index++], elapsed);
    }
}

int CollectionObjectOperations::UpdateRenderParameters(
    CollectionState& state, double elapsed, ICollectionUpdateRuntime& runtime) {
    return runtime.UpdateRenderParameters(state, static_cast<float>(elapsed));
}

}  // namespace pal4
