#include "pal4/script_poetry_wait.h"

namespace pal4 {

std::uint32_t ScriptPoetryWait::Show(std::uint32_t poetry_id,
                                     std::uint32_t wait_mode) {
    runtime_.CreateAndHidePoetryWindow(runtime_.UiFrameManager(), poetry_id);
    const auto frame = runtime_.PoetryWindow(runtime_.UiFrameManager());
    const auto mode = static_cast<std::uint8_t>(wait_mode);
    if (mode == 0u) return frame;

    auto* state = runtime_.AllocateWait(sizeof(TutorialWaitState));
    if (state != nullptr) {
        runtime_.ConstructWait(*state, mode,
                               runtime_.ScriptExecutor() + kQueueOffset);
        state->vtable_address = kWaitVtable;
        state->frame = frame;
        runtime_.FlushWait(*state);
    }
    return runtime_.AddHandler(runtime_.ScriptExecutor() + kQueueOffset,
                               state);
}

}  // namespace pal4
