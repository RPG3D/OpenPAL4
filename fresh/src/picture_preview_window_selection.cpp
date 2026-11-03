#include "pal4/picture_preview_window_selection.h"

#include "pal4/preview_text_codec.h"

namespace pal4 {

void PicturePreviewWindowSelection::UpdateStaticInfoByIndex(
    PicturePreviewWindowState& state, std::int32_t index,
    IPicturePreviewWindowSelectionRuntime& runtime) {
    if (index >= 0 && index <= 10)
        runtime.UpdateStaticInfo(
            state, state.Read(0x2cu + static_cast<std::uint32_t>(index) * 4u));
}

void PicturePreviewWindowSelection::UpdateStaticInfo(
    PicturePreviewWindowState& state, std::uint32_t selected,
    IPicturePreviewWindowSelectionRuntime& runtime) {
    const auto mask = state.Read(0x5cu);
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
    for (std::uint32_t i = 0u; i < 10u; ++i) {
        if (state.Read(0x2cu + i * 4u) == selected) {
            runtime.WriteWord(mask + 0x118u, i);
            break;
        }
    }
    const auto task_index = runtime.ReadWord(selected + 0x118u);
    if (!runtime.HasTriggerTask(task_index)) return;
    const auto raw = runtime.TriggerTaskText(task_index);
    const auto utf8 = runtime.ConvertToUtf8(raw);
    runtime.SetText(state.Read(0x54u),
                    PreviewTextCodec::DecodeUtf8(utf8));
}

std::uint8_t PicturePreviewWindowSelection::OnMouseMove(
    PicturePreviewWindowState& state, std::uint32_t event,
    IPicturePreviewWindowSelectionRuntime& runtime) {
    runtime.UpdateStaticInfo(state, runtime.ReadWord(event + 8u));
    return 1u;
}

std::uint8_t PicturePreviewWindowSelection::OnMouseClick(
    PicturePreviewWindowState& state, std::uint32_t event,
    IPicturePreviewWindowSelectionRuntime& runtime) {
    if (runtime.ReadWord(event + 0x1cu) == 0u)
        runtime.HandleItemClick(state, runtime.ReadWord(event + 8u));
    return 1u;
}

std::uint8_t PicturePreviewWindowSelection::HandleItemClick(
    PicturePreviewWindowState& state, std::uint32_t selected,
    IPicturePreviewWindowSelectionRuntime& runtime) {
    const auto index = runtime.ReadWord(selected + 0x118u);
    if (!runtime.HasTriggerTask(index)) return 0u;
    const auto task = runtime.TriggerTask(index);
    runtime.Hide(state, 0u, 0u);
    return runtime.DispatchPictureView(1u, task);
}

}  // namespace pal4
