#include "pal4/movie_play_window_lifecycle.h"

namespace pal4 {

MoviePlayWindowState& MoviePlayWindowLifecycle::Construct(
    MoviePlayWindowState& state, std::uint32_t name,
    IMoviePlayWindowLifecycleRuntime& runtime) {
    runtime.LoadLayout(state, name, 0u, 0u);
    try {
        state.Write(0x28u, kBaseMemberVtable);
        runtime.ConstructVideoPath(state, 0x38u);
        try {
            state.Write(0u, kObjectVtable);
            state.Write(0x28u, kMemberVtable);
            runtime.InitializeButtons(state);
        } catch (...) {
            runtime.CleanupVideoPath(state, 0x38u);
            throw;
        }
    } catch (...) {
        runtime.DestroyDialog(state);
        throw;
    }
    return state;
}

void MoviePlayWindowLifecycle::Destroy(
    MoviePlayWindowState& state,
    IMoviePlayWindowLifecycleRuntime& runtime) {
    runtime.CleanupVideoPath(state, 0x38u);
    runtime.DestroyDialog(state);
}

}  // namespace pal4
