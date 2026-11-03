#include "pal4/ui_frame_manager_singleton.h"

namespace pal4 {

std::int32_t UiFrameManagerSingleton::SetPortraitTimerDuration(
    const std::uint32_t manager, const std::int32_t duration,
    IUiFrameManagerTimerControlRuntime& runtime) {
    const auto frame = runtime.FindFrame(manager, "portrait");
    if (frame == 0u) return 0;
    if (duration > 0 && runtime.ReadWord(frame + 0xe8u) != 0u)
        runtime.WriteWord(frame + 0xf0u,
                          static_cast<std::uint32_t>(duration));
    return duration;
}

void UiFrameManagerSingleton::ClearPortraitTimerText(
    const std::uint32_t manager,
    IUiFrameManagerTimerControlRuntime& runtime) {
    const auto frame = runtime.FindFrame(manager, "portrait");
    if (frame != 0u) runtime.ClearTimerAndText(frame);
}

std::int32_t UiFrameManagerSingleton::GetPortraitTimerDuration(
    const std::uint32_t manager,
    IUiFrameManagerTimerControlRuntime& runtime) {
    const auto frame = runtime.FindFrame(manager, "portrait");
    return frame == 0u ? 0
                       : static_cast<std::int32_t>(
                             runtime.ReadWord(frame + 0xf0u));
}

}  // namespace pal4
