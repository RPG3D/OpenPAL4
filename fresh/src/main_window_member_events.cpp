#include "pal4/main_window_member_events.h"

namespace pal4 {

std::uint8_t MainWindowMemberEvents::ForwardKey(
    MainWindowMemberState& state, std::uint32_t event,
    IMainWindowMemberEventRuntime& runtime) {
    return runtime.HandleKeyEvent(state, event);
}

std::uint8_t MainWindowMemberEvents::ResetSelection(
    MainWindowMemberState& state,
    IMainWindowMemberEventRuntime& runtime) {
    if (state.bytes[0x40u] == 0u) return runtime.HandleSelection(state);
    state.bytes[0x40u] = 0u;
    return 1u;
}

}  // namespace pal4
