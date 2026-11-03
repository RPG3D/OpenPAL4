#include "pal4/loading_window.h"

#include <cstdint>

namespace pal4 {

LoadingWindowState& LoadingWindow::Construct(
    LoadingWindowState& state, ILoadingWindowRuntime& runtime) {
    runtime.LoadLayout(state, "loading", 0u, 0u);
    state.vtable = kVtable;
    state.poser = runtime.GetWindow(state.layout_prefix + "ImgPoser");
    state.mask = runtime.GetWindow(state.layout_prefix + "ImgMask");
    state.mask_x = runtime.GetXPosition(state.mask);
    state.mask_width = runtime.GetWidth(state.mask);
    state.poser_x = runtime.GetXPosition(state.poser);
    return state;
}

void LoadingWindow::UpdateProgress(
    LoadingWindowState& state, float progress,
    ILoadingWindowRuntime& runtime) {
    const auto geometry = ProgressGeometry(state, progress);
    runtime.SetWidth(state.mask, geometry.mask_width);
    runtime.SetXPosition(state.mask, geometry.mask_x);
    runtime.SetXPosition(state.poser, geometry.poser_x);
}

LoadingWindowGeometry LoadingWindow::ProgressGeometry(
    const LoadingWindowState& state, float progress) {
    const auto offset = progress * state.mask_width;
    return {state.mask_width - offset, state.mask_x + offset,
            state.poser_x + progress * 0.32249999f};
}

void LoadingWindow::SetVisible(
    LoadingWindowState& state, bool visible, std::uint32_t parent,
    ILoadingWindowRuntime& runtime) {
    UpdateProgress(state, 0.0f, runtime);
    runtime.SetParent(state, visible, parent);
}

}  // namespace pal4
