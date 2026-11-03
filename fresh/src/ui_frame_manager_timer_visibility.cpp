#include "pal4/ui_frame_manager_singleton.h"

namespace pal4 {

bool UiFrameManagerSingleton::IsPortraitTimerVisible(
    const std::uint32_t manager,
    IUiFrameManagerVisibilityRuntime& runtime) {
    const auto frame = runtime.FindFrame(manager, "portrait");
    return frame == 0u || runtime.IsVisible(frame);
}

}  // namespace pal4
