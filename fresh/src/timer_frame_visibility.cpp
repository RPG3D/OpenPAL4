#include "pal4/timer_frame_visibility.h"

namespace pal4 {

bool TimerFrameVisibility::SetVisible(
    const std::uint32_t enabled, const std::uint32_t selection,
    ITimerFrameVisibilityRuntime& runtime) {
    const auto name = runtime.ReadWord(kNameSource);
    const auto manager = runtime.FrameManager();
    auto frame = runtime.FindFrame(manager, name);
    if (enabled != 0u) {
        if (frame == 0u) {
            runtime.RegisterFrame(runtime.FrameManager(), name);
            frame = runtime.FindFrame(runtime.FrameManager(), name);
        }
        if (frame == 0u) {
            return false;
        }
        runtime.WriteWord(frame + 0x74u, selection);
        runtime.SetFrameVisibility(frame, 1u, 0u);
        return true;
    }
    if (frame != 0u) {
        runtime.SetFrameVisibility(frame, 0u, 0u);
    }
    return frame != 0u;
}

}  // namespace pal4
