#include "pal4/ui_frame_event_handler.h"

namespace pal4 {

bool UiFrameEventHandler::Handle(const std::uint32_t name,
                                 const UiFrameEvent event,
                                 IUiFrameEventRuntime& runtime) {
    auto frame = runtime.FindFrame(runtime.FrameManager(), name);
    if (event.enabled != 0u && frame == 0u) {
        runtime.RegisterFrame(runtime.FrameManager(), name);
        frame = runtime.FindFrame(runtime.FrameManager(), name);
    }
    if (frame == 0u) return false;
    runtime.SetVisible(frame, event.enabled != 0u ? 1u : 0u,
                       event.argument);
    return true;
}

}  // namespace pal4
