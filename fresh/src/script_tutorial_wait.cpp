#include "pal4/script_tutorial_wait.h"

namespace pal4 {

std::uint32_t ScriptTutorialWait::Show(std::uint32_t key) {
    runtime_.ShowTutorial(runtime_.UiFrameManager(), key);
    const auto frame = runtime_.FindFrame(runtime_.UiFrameManager(),
                                          kFrameName);
    auto* state = runtime_.AllocateWait(sizeof(TutorialWaitState));
    if (state != nullptr) {
        runtime_.ConstructWait(*state, 1u,
                               runtime_.ScriptExecutor() + kQueueOffset);
        state->vtable_address = kWaitVtable;
        state->frame = frame;
        runtime_.FlushWait(*state);
    }
    return runtime_.AddHandler(runtime_.ScriptExecutor() + kQueueOffset,
                               state);
}

void ScriptTutorialWait::Update(TutorialWaitState& state) {
    if (state.frame == 0u || !runtime_.IsVisible(state.frame))
        runtime_.CloseWait(state);
}

}  // namespace pal4
