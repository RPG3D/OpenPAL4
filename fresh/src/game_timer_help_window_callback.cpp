#include "pal4/game_timer_help_window_callback.h"

namespace pal4 {

std::uint8_t GameTimerHelpWindowCallback::Tick(
    IGameTimerHelpWindowRuntime& runtime) const {
    runtime.WriteDword(address_, runtime.ReadDword(address_) + 1u);
    const auto frame = runtime.FindFrame(runtime.UiFrameManager(), "gameInfo");
    return frame == 0u ? 0u : runtime.UpdateClock(frame);
}

}  // namespace pal4
