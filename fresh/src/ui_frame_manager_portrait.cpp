#include "pal4/ui_frame_manager_singleton.h"

namespace pal4 {

void UiFrameManagerSingleton::SetPortraitFlag(
    const std::uint32_t manager, const std::uint8_t flag,
    IUiFrameManagerPortraitRuntime& runtime) {
    const auto frame = runtime.FindFrame(manager, "portrait");
    if (frame != 0u) runtime.SetFlagAndUpdateVisibility(frame, flag);
}

}  // namespace pal4
