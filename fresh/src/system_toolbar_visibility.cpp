#include "pal4/system_toolbar_visibility.h"

namespace pal4 {

std::uint32_t SystemToolbarVisibility::Update(
    const std::uint32_t frame, const std::uint8_t visible,
    const std::uint32_t parent, ISystemToolbarVisibilityRuntime& runtime) {
    runtime.SetParent(frame, visible, parent);
    runtime.DispatchEvent(runtime.FrameManager() + 260u, kParentEvent,
                          visible, 0u);
    runtime.DispatchEvent(runtime.FrameManager() + 260u, kWindowEvent,
                          visible, runtime.ReadWord(frame + 36u));

    const auto member = frame + 40u;
    if (visible == 0u) {
        return runtime.RemoveMember(runtime.FrameManager() + 308u, member);
    }

    constexpr struct GameButton {
        std::uint32_t offset;
        std::uint32_t entry;
    } buttons[] = {
        {72u, 8u}, {64u, 9u}, {76u, 10u},
        {68u, 11u}, {80u, 13u}, {84u, 14u}};
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto& button = buttons[index];
        const auto enabled = runtime.GameEntry(runtime.Game(),
                                               button.entry) != 0;
        runtime.SetEnabled(runtime.ReadWord(frame + button.offset), enabled);
    }
    const auto player = runtime.PlayerByIndex(runtime.PlayerManager(), 3u);
    const auto forging_enabled = player != 0u &&
        runtime.ReadWord(player + 0xb08u) != 0u;
    runtime.SetEnabled(runtime.ReadWord(frame + 88u), forging_enabled);
    for (std::uint32_t index = 4u; index < 6u; ++index) {
        const auto& button = buttons[index];
        const auto enabled = runtime.GameEntry(runtime.Game(),
                                               button.entry) != 0;
        runtime.SetEnabled(runtime.ReadWord(frame + button.offset), enabled);
    }
    runtime.DispatchEvent(runtime.FrameManager() + 260u, kShowEvent, 5u, 0u);
    const auto last = runtime.LastActiveMember(runtime.FrameManager() + 308u);
    if (last == member) {
        return last;
    }
    return runtime.InsertMember(runtime.FrameManager() + 308u, member);
}

}  // namespace pal4
