#include "pal4/movie_preview_window_images.h"

namespace pal4 {

void MoviePreviewWindowImages::SetImageAndDisable(
    MoviePreviewWindowState& state, std::uint32_t image,
    IMoviePreviewWindowImagesRuntime& runtime) {
    runtime.SetImagePointer(image, state.Read(0x6cu));
    runtime.SetEnabled(image, false);
}

void MoviePreviewWindowImages::Update(
    MoviePreviewWindowState& state,
    IMoviePreviewWindowImagesRuntime& runtime) {
    const auto unlocked = runtime.UnlockMask();
    for (std::uint32_t i = 0u; i < 13u; ++i) {
        const auto image = state.Read(0x2cu + i * 4u);
        if (!runtime.HasTriggerTask(i)) {
            SetImageAndDisable(state, image, runtime);
            continue;
        }
        const auto task = runtime.TriggerTaskAddress(i);
        if ((unlocked & (1u << i)) == 0u) {
            SetImageAndDisable(state, image, runtime);
            continue;
        }
        runtime.SetEnabled(image, true);
        runtime.SetVisible(image, true);
        const auto raw = runtime.TriggerTaskImageName(task);
        const auto utf8 = runtime.ConvertToUtf8(raw);
        const auto nul = utf8.find('\0');
        runtime.SetImage(image, kImageset,
                         std::string_view(utf8.data(),
                                          nul == std::string::npos ?
                                              utf8.size() : nul));
        runtime.WriteWord(image + 0x118u, runtime.TriggerTaskId(task));
    }
}

std::uint8_t MoviePreviewWindowImages::HandleItemClick(
    MoviePreviewWindowState& state, std::uint32_t selected,
    IMoviePreviewWindowImagesRuntime& runtime) {
    const auto index = runtime.ReadWord(selected + 0x118u);
    if (!runtime.HasTriggerTask(index)) return 0u;
    const auto task = runtime.TriggerTaskAddress(index);
    runtime.Hide(state, 0u, 0u);
    return runtime.Dispatch(kPlayCommand, 1u, task);
}

}  // namespace pal4
