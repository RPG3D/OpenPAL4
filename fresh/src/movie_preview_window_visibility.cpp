#include "pal4/movie_preview_window_visibility.h"

namespace pal4 {

std::uint32_t MoviePreviewWindowVisibility::SetVisible(
    MoviePreviewWindowState& state, std::uint32_t object_address,
    std::uint8_t showing, std::uint32_t parent,
    IMoviePreviewWindowVisibilityRuntime& runtime) {
    runtime.SetParent(state, showing, parent);
    if (showing != 0u) {
        runtime.UpdateImages(state);
        const auto first_image = state.Read(0x2cu);
        if (!runtime.IsDisabled(first_image, false))
            runtime.UpdateStaticInfo(state, first_image);
        const auto manager = runtime.FrameManager();
        return runtime.InsertActive(manager + 0x134u,
                                    object_address + 0x28u);
    }
    const auto member = object_address == 0u ? 0u : object_address + 0x28u;
    const auto manager = runtime.FrameManager();
    return runtime.RemoveActive(manager + 0x134u, member);
}

}  // namespace pal4
