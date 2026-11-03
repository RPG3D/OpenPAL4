#include "pal4/grid_manager.h"

namespace pal4 {
namespace {

constexpr std::uint32_t kVtable = 0x84b600u;

}  // namespace

GridManagerState& GridManager::Construct(
    GridManagerState& state, IGridManagerRuntime& runtime) {
    (void)ConstructBase(state, runtime);
    state.base_words[0] = kVtable;
    state.nested_grid = 0u;
    state.owned_memory = 0u;
    state.reserved_a = 0u;
    state.reserved_b = 0u;
    state.update_enabled = 0u;
    state.reset_pending = 0u;
    return state;
}

GridManagerState& GridManager::Destroy(
    GridManagerState& state, const bool free_self,
    IGridManagerRuntime& runtime) {
    Cleanup(state, runtime);
    if (free_self)
        runtime.FreeSelf(state);
    return state;
}

void GridManager::Cleanup(
    GridManagerState& state, IGridManagerRuntime& runtime) {
    state.base_words[0] = kVtable;
    if (state.nested_grid != 0u) {
        runtime.DestroyNested(state.nested_grid, true);
        state.nested_grid = 0u;
    }
    (void)FreeMemory(state, runtime);
    SetBaseVtable(state);
}

std::uint32_t GridManager::FreeMemory(
    GridManagerState& state, IGridManagerRuntime& runtime) {
    if (state.owned_memory == 0u)
        return 0u;
    const auto result = runtime.Free(state.owned_memory);
    state.owned_memory = 0u;
    return result;
}

}  // namespace pal4
