#include "pal4/picture_view_window_commands.h"

namespace pal4 {

std::uint8_t PictureViewWindowCommands::HandleViewCommand(
    std::uint32_t visible, std::uint32_t record,
    IPictureViewWindowCommandsRuntime& runtime) {
    runtime.SetFrameVisible(kWindow, visible != 0u);
    if (visible != 0u) {
        runtime.Display(runtime.FindFrame(kWindow), record);
    } else {
        runtime.Dispatch(kPreviewCommand, 1u, 0u);
    }
    return 1u;
}

std::uint8_t PictureViewWindowCommands::Close(
    PictureViewWindowState& state, IPictureViewWindowCommandsRuntime& runtime) {
    if (runtime.HasImageset(kImageset)) runtime.DestroyImageset(kImageset);
    const auto index = state.Read(0x60u);
    runtime.Hide(state, 0u, 0u);
    runtime.Dispatch(kPreviewCommand, 1u, 0u);
    const auto preview = runtime.FindFrame(kPreviewWindow);
    if (preview != 0u) runtime.UpdatePreview(preview, index);
    return 1u;
}

std::uint8_t PictureViewWindowCommands::Previous(
    PictureViewWindowState& state, std::uint32_t frame,
    IPictureViewWindowCommandsRuntime& runtime) {
    auto index = state.Read(0x60u) - 1u;
    if (static_cast<std::int32_t>(index) < 0) index = 0u;
    state.Write(0x60u, index);
    const auto begin = state.Read(0x54u);
    if (begin != 0u && state.Read(0x58u) != begin)
        runtime.Display(frame, runtime.ReadWord(begin + index * 4u));
    return 1u;
}

std::uint8_t PictureViewWindowCommands::Next(
    PictureViewWindowState& state, std::uint32_t frame,
    IPictureViewWindowCommandsRuntime& runtime) {
    auto index = state.Read(0x60u) + 1u;
    state.Write(0x60u, index);
    const auto begin = state.Read(0x54u);
    const auto count = begin == 0u ? 0u :
        static_cast<std::uint32_t>(
            static_cast<std::int32_t>(state.Read(0x58u) - begin) >> 2);
    if (begin == 0u || index >= count) {
        index = count - 1u;
        state.Write(0x60u, index);
    }
    if (begin != 0u && count != 0u)
        runtime.Display(frame, runtime.ReadWord(begin + index * 4u));
    return 1u;
}

std::uint32_t PictureViewWindowCommands::OnCloseMemberEvent(
    PictureViewWindowState& state, std::uint32_t member,
    std::uint32_t event_code, std::uint32_t event,
    IPictureViewWindowCommandsRuntime& runtime) {
    if (event_code != 1u)
        return runtime.HandleOtherMemberEvent(member, event);
    (void)Close(state, runtime);
    return 1u;
}

}  // namespace pal4
