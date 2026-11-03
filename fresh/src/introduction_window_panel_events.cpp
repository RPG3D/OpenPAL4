#include "pal4/introduction_window_panel_events.h"

namespace pal4 {

bool IntroductionWindowPanelEvents::Handle(
    IntroductionWindowState& state, std::uint32_t member_address,
    std::uint32_t event_address,
    IIntroductionWindowPanelEventsRuntime& runtime) {
    const auto event_code = runtime.ReadEventCode(event_address + 0x10u);
    switch (event_code) {
    case 1u: {
        const auto manager = runtime.FrameManager();
        runtime.RemoveActive(manager + 0x134u, member_address);
        IntroductionWindowVisibility::SetPanelVisibility(state, false,
                                                          runtime);
        return true;
    }
    case 203u:
        for (int index = 4; index >= 0; --index) {
            const auto window = state.Read(0x98u +
                static_cast<std::uint32_t>(index) * 4u);
            if (runtime.IsVisible(window) && runtime.IsDisabled(window)) {
                if (index > 0) {
                    runtime.HandleMouseEvent(state.Read(0x98u +
                        static_cast<std::uint32_t>(index - 1) * 4u));
                }
                break;
            }
        }
        return true;
    case 205u:
        for (std::uint32_t index = 0u; index < 5u; ++index) {
            const auto window = state.Read(0x98u + index * 4u);
            if (runtime.IsVisible(window) && runtime.IsDisabled(window)) {
                if (index + 1u < 5u) {
                    runtime.HandleMouseEvent(state.Read(0x98u +
                        (index + 1u) * 4u));
                }
                break;
            }
        }
        return true;
    case 200u:
    case 208u: {
        const auto scrollbar = runtime.ScrollbarOf(state.Read(0x80u));
        const auto position = runtime.ScrollPosition(scrollbar);
        const auto delta = event_code == 200u ? -10.0f : 10.0f;
        runtime.SetScrollPosition(scrollbar, position + delta);
        return true;
    }
    default:
        return false;
    }
}

}  // namespace pal4
