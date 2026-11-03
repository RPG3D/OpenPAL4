#include "pal4/system_setting_frame_mode.h"

namespace pal4 {
namespace {

void SetWindowGroups(const std::uint32_t frame, const bool setting_visible,
                     const bool save_visible,
                     ISystemSettingDisplayRuntime& runtime) {
    for (std::uint32_t index = 0; index < 3u; ++index) {
        runtime.SetVisible(runtime.ReadWord(frame + 112u + 4u * index),
                           setting_visible);
    }
    for (std::uint32_t index = 0; index < 3u; ++index) {
        runtime.SetVisible(runtime.ReadWord(frame + 124u + 4u * index),
                           save_visible);
    }
}

void SwitchActive(const std::uint32_t frame,
                  const std::uint32_t remove_offset,
                  const std::uint32_t insert_offset,
                  ISystemSettingOptionsRuntime& runtime) {
    runtime.RemoveActive(runtime.FrameManager() + 308u,
                         frame + remove_offset);
    runtime.InsertActive(runtime.FrameManager() + 308u,
                         frame + insert_offset);
}

std::uint32_t ShowBlank(const std::uint32_t frame, const bool visible,
                        ISystemSettingDisplayRuntime& runtime) {
    const auto buffer = runtime.BeginName(frame);
    try {
        const auto name = runtime.ResetName(buffer, frame,
                                            "Savewindowblanknew");
        runtime.SetVisible(runtime.FindWindow(name), visible);
    } catch (...) {
        (void)runtime.EndName(buffer);
        throw;
    }
    return runtime.EndName(buffer);
}

}  // namespace

std::uint32_t SystemSettingFrameMode::ShowSave(
    const std::uint32_t frame, ISystemSettingOptionsRuntime& runtime) {
    SetWindowGroups(frame, true, false, runtime);
    const auto slider = runtime.ReadWord(frame + 496u);
    runtime.WriteWord(frame + 92u, 0u);
    runtime.WriteWord(frame + 88u, slider);
    SwitchActive(frame, 96u, 64u, runtime);
    return ShowBlank(frame, true, runtime);
}

std::uint32_t SystemSettingFrameMode::ShowDataMode(
    const std::uint32_t frame, ISystemSettingFrameModeRuntime& runtime) {
    SetWindowGroups(frame, false, true, runtime);
    const auto data = runtime.GlobalData();
    const auto slot = static_cast<std::int32_t>(runtime.ReadWord(data + 44u));
    runtime.PopulateSaveList(frame, slot / 10, 1u);
    SwitchActive(frame, 64u, 96u, runtime);
    return ShowBlank(frame, true, runtime);
}

std::uint32_t SystemSettingFrameMode::ShowSystem(
    const std::uint32_t frame, ISystemSettingDisplayRuntime& runtime) {
    SetWindowGroups(frame, false, false, runtime);
    for (std::uint32_t index = 0; index < 4u; ++index) {
        runtime.SetSelected(runtime.ReadWord(frame + 184u + 4u * index),
                            false);
    }
    runtime.SetImage(runtime.ReadWord(frame + 180u), "zi0", "xitong");
    return ShowBlank(frame, false, runtime);
}

}  // namespace pal4
