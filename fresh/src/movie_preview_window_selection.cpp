#include "pal4/movie_preview_window_selection.h"
#include "pal4/preview_text_codec.h"

#include <stdexcept>

namespace pal4 {

std::u32string MoviePreviewWindowSelection::DecodeUtf8(
    std::string_view text) {
    return PreviewTextCodec::DecodeUtf8(text);
}

void MoviePreviewWindowSelection::UpdateStaticInfo(
    MoviePreviewWindowState& state, std::uint32_t selected,
    IMoviePreviewWindowSelectionRuntime& runtime) {
    const auto mask = state.Read(0x68u);
    if (selected == 0u) {
        runtime.SetVisible(mask, false);
        return;
    }
    if (runtime.IsDisabled(selected, false)) return;

    const auto old_parent = runtime.ReadWord(mask + 0x44u);
    if (old_parent != 0u) runtime.RemoveChild(old_parent, mask);
    runtime.AddChild(selected, mask);
    runtime.SetVisible(mask, true);
    runtime.SetWidth(mask, 0u, 1.0f);
    runtime.SetHeight(mask, 0u, 1.0f);
    runtime.SetX(mask, 0u, 0.0f);
    runtime.SetY(mask, 0u, 0.0f);

    runtime.WriteWord(mask + 0x118u, 0u);
    for (std::uint32_t i = 0u; i < 13u; ++i) {
        if (state.Read(0x2cu + i * 4u) == selected) {
            runtime.WriteWord(mask + 0x118u, i);
            break;
        }
    }
    const auto task = runtime.ReadWord(selected + 0x118u);
    if (!runtime.HasTriggerTask(task)) return;
    const auto raw = runtime.TriggerTaskText(task);
    const auto utf8 = runtime.ConvertToUtf8(raw);
    runtime.SetText(state.Read(0x60u), DecodeUtf8(utf8));
}

std::uint8_t MoviePreviewWindowSelection::OnMouseMove(
    MoviePreviewWindowState& state, std::uint32_t event,
    IMoviePreviewWindowSelectionRuntime& runtime) {
    UpdateStaticInfo(state, runtime.ReadWord(event + 8u), runtime);
    return 1u;
}

std::uint8_t MoviePreviewWindowSelection::OnMouseClick(
    MoviePreviewWindowState& state, std::uint32_t event,
    IMoviePreviewWindowSelectionRuntime& runtime) {
    if (runtime.ReadWord(event + 0x1cu) == 0u)
        runtime.HandleItemClick(state, runtime.ReadWord(event + 8u));
    return 1u;
}

}  // namespace pal4
