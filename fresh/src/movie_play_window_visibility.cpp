#include "pal4/movie_play_window_visibility.h"

namespace pal4 {

void MoviePlayWindowVisibility::SetVisible(
    MoviePlayWindowState& state, std::uint32_t object_address,
    std::uint8_t showing, std::uint32_t parent,
    IMoviePlayWindowVisibilityRuntime& runtime) {
    runtime.SetParent(state, showing, parent);
    const auto manager = runtime.FrameManager();
    const auto member = object_address + 0x28u;
    if (showing != 0u) {
        runtime.InsertActive(manager + 0x134u, member);
        runtime.StopCurrentWithFade(runtime.MusicPlayer(), 1.0f);
    } else {
        runtime.RemoveActive(manager + 0x134u, member);
        runtime.FadeInBackground(runtime.MusicPlayer(), "P01-1.mp3",
                                 3.0f, 0u);
    }
}

}  // namespace pal4
