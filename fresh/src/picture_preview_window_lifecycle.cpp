#include "pal4/picture_preview_window_lifecycle.h"

namespace pal4 {

PicturePreviewWindowState& PicturePreviewWindowLifecycle::Construct(
    PicturePreviewWindowState& state, std::uint32_t name,
    IPicturePreviewWindowLifecycleRuntime& runtime) {
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

}  // namespace pal4
