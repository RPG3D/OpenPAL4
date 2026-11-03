#include "pal4/pal_test_window_member_events.h"

namespace pal4 {

std::uint8_t PalTestWindowMemberEvents::OnEvent(
    std::uint32_t member, std::uint32_t event,
    IPalTestWindowMemberEventRuntime& runtime) {
    if (runtime.ReadWord(event + 0x10u) != 1u)
        return runtime.HandleKeyEvent(member, event);
    runtime.CloseWithDefaultEventArgs(member - 0x28u);
    return 1u;
}

}  // namespace pal4
