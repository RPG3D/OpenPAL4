#include "pal4/script_world_map_wait.h"

namespace pal4 {

std::uint32_t ScriptWorldMapWait::Show() {
    runtime_.ProcessWorldMapEvent(runtime_.UiFrameManager(), 1u, 0u);
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
    runtime_.AddHandler(runtime_.ScriptExecutor() + kQueueOffset, state);

    const auto document = runtime_.CreateDocument();
    try {
        runtime_.EnterState(11u, document);
    } catch (...) {
        static_cast<void>(runtime_.DestroyDocument(document));
        throw;
    }
    return runtime_.DestroyDocument(document);
}

}  // namespace pal4
