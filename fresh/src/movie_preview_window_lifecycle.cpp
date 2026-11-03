#include "pal4/movie_preview_window_lifecycle.h"

namespace pal4 {

MoviePreviewWindowState& MoviePreviewWindowLifecycle::Construct(
    MoviePreviewWindowState& state, std::uint32_t name,
    IMoviePreviewWindowLifecycleRuntime& runtime) {
    runtime.LoadLayout(state, name, 0u, 0u);
    state.Write(0x28u, kInitialMemberVtable);
    state.Write(0u, kObjectVtable);
    state.Write(0x28u, kMemberVtable);
    try {
        runtime.InitializeComponents(state);
    } catch (...) {
        runtime.DestroyDialogBase(state);
        throw;
    }
    return state;
}

std::uint32_t MoviePreviewWindowLifecycle::Destroy(
    MoviePreviewWindowState& state, std::uint32_t object_address,
    std::uint32_t flags, IMoviePreviewWindowLifecycleRuntime& runtime) {
    runtime.DestroyWindow(state);
    if ((flags & 1u) != 0u) runtime.FreeMemory(object_address);
    return object_address;
}

}  // namespace pal4
