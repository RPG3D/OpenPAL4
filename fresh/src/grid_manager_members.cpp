#include "pal4/grid_manager_members.h"

namespace pal4 {

GridManagerMembersState& GridManagerMembers::Initialize(
    GridManagerMembersState& state) {
    state.first_flag = 0u;
    state.elements_begin = 0u;
    state.elements_end = 0u;
    state.elements_capacity = 0u;
    state.second_flag = 0u;
    state.secondary_begin = 0u;
    state.secondary_end = 0u;
    state.secondary_capacity = 0u;
    state.grid_wrapper = 0u;
    state.grid_ready = 0u;
    state.update_disabled = 0u;
    return state;
}

std::uint32_t GridManagerMembers::Cleanup(
    GridManagerMembersState& state,
    IGridManagerMembersRuntime& runtime) {
    ResetGrid(state, runtime);
    (void)runtime.Free(state.secondary_begin);
    state.secondary_begin = 0u;
    state.secondary_end = 0u;
    state.secondary_capacity = 0u;
    const auto result = runtime.Free(state.elements_begin);
    state.elements_begin = 0u;
    state.elements_end = 0u;
    state.elements_capacity = 0u;
    return result;
}

std::uint32_t GridManagerMembers::InitializeGrid(
    GridManagerMembersState& state,
    IGridManagerMembersRuntime& runtime) {
    if (state.grid_wrapper != 0u)
        runtime.DestroyGridWrapper(state.grid_wrapper);
    state.grid_wrapper = 0u;
    const auto wrapper = runtime.Allocate(8u);
    std::uint32_t grid = 0u;
    if (wrapper != 0u) {
        // IDA65E479..65E4C4: floor(20.5), floor(1.5), floor(20.5);
        // preserve the three x87/__ftol values, not the decompiler v5 alias.
        const SpatialGridConfig config{{-40.0f, -40.0f, -40.0f},
                                       {80.0f, 80.0f, 80.0f},
                                       {20u, 1u, 20u}};
        grid = SpatialGrid::Create(config, runtime);
        runtime.WriteWord(wrapper, 0x84b6b4u);
        runtime.WriteGridWrapper(wrapper, grid);
        state.grid_wrapper = wrapper;
    }
    state.grid_ready = 1u;
    state.update_disabled = 0u;
    return grid;
}

void GridManagerMembers::ResetGrid(
    GridManagerMembersState& state,
    IGridManagerMembersRuntime& runtime) {
    if (state.grid_wrapper != 0u)
        runtime.DestroyGridWrapper(state.grid_wrapper);
    state.grid_wrapper = 0u;
    state.grid_ready = 0u;
    state.update_disabled = 1u;
}

}  // namespace pal4
