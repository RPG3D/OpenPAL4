#include "pal4/ui_frame_manager_singleton.h"

namespace pal4 {

void UiFrameManagerSingleton::ShowTutorialDialog(
    const std::uint32_t manager, const std::uint32_t key,
    IUiFrameManagerTutorialRuntime& runtime) {
    const auto data = runtime.InitializeGlobalData();
    if (runtime.ReadWord(data + 8u) == 0u) return;

    const auto flag_data = runtime.InitializeGlobalData();
    if (runtime.GetDialogFlag(flag_data, key)) return;

    auto frame = runtime.FindFrame(manager, "TutorialWindow");
    if (frame == 0u) {
        const auto instance = runtime.ManagerSingleton();
        runtime.RegisterFrame(instance, "TutorialWindow");
        frame = runtime.FindFrame(manager, "TutorialWindow");
        if (frame == 0u) return;
    }

    const auto save_data = runtime.InitializeGlobalData();
    runtime.SetDialogFlagAndSave(save_data, key);
    runtime.UpdateVisibility(frame, 1u, key, 0u);
}

std::uint8_t UiFrameManagerSingleton::GetPortraitTimerFlag(
    const std::uint32_t manager,
    IUiFrameManagerTimerFlagRuntime& runtime) {
    const auto frame = runtime.FindFrame(manager, "portrait");
    return frame == 0u ? 0u : runtime.ReadByte(frame + 0xf6u);
}

}  // namespace pal4
