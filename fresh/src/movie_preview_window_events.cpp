#include "pal4/movie_preview_window_events.h"

namespace pal4 {

std::uint8_t MoviePreviewWindowEvents::OnMemberEvent(
    MoviePreviewWindowState& state, std::uint32_t member_address,
    std::uint32_t event_address,
    IMoviePreviewWindowEventRuntime& runtime) {
    const auto code = runtime.ReadWord(event_address + 0x10u);
    if (code == 1u) {
        const auto base_event = runtime.ConstructBaseEvent();
        try {
            runtime.Close(state, member_address - 0x28u, base_event);
        } catch (...) {
            runtime.DestroyBaseEvent(base_event);
            throw;
        }
        runtime.DestroyBaseEvent(base_event);
        return 1u;
    }

    const auto index = runtime.ReadWord(state.Read(0x68u) + 0x118u);
    switch (code) {
    case 28u:
        if (index <= 12u)
            runtime.HandleItemClick(state, state.Read(0x2cu + index * 4u));
        return 1u;
    case 203u:
        if (static_cast<std::int32_t>(index) > 0)
            runtime.UpdateStaticInfo(
                state, state.Read(0x2cu + (index - 1u) * 4u));
        return 1u;
    case 205u:
        if (static_cast<std::int32_t>(index) < 12)
            runtime.UpdateStaticInfo(
                state, state.Read(0x2cu + (index + 1u) * 4u));
        return 1u;
    case 200u:
        if (static_cast<std::int32_t>(index) >= 4)
            runtime.UpdateStaticInfo(
                state, state.Read(0x2cu + (index - 4u) * 4u));
        return 1u;
    case 208u:
        if (static_cast<std::int32_t>(index) < 9)
            runtime.UpdateStaticInfo(
                state, state.Read(0x2cu + (index + 4u) * 4u));
        return 1u;
    default:
        return 0u;
    }
}

}  // namespace pal4
