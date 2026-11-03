#include "pal4/script_game_init_wait.h"

namespace pal4 {

std::uint32_t ScriptGameInitWait::Start() {
    runtime_.InitializeStateAndUi(runtime_.Game(), 0u, 0u);
    auto* state = runtime_.AllocateWait(sizeof(GameInitWaitState));
    if (state != nullptr) {
        runtime_.ConstructWait(*state, 1u,
                               runtime_.ScriptExecutor() + kQueueOffset);
        state->vtable_address = kVtable;
        state->state_identity = 0u;
        runtime_.FlushWait(*state);
    }
    return runtime_.AddHandler(runtime_.ScriptExecutor() + kQueueOffset,
                               state);
}

void ScriptGameInitWait::Update(GameInitWaitState& state) {
    const auto current = runtime_.CurrentState(runtime_.Game());
    if (current == 0u ||
        runtime_.StateIdentity(current) == state.state_identity)
        Close(state);
}

void ScriptGameInitWait::Close(GameInitWaitState& state) {
    runtime_.CloseWait(state);
}

std::uint32_t ScriptGameInitWait::Destroy(GameInitWaitState& state) {
    return runtime_.DestroyWait(state);
}

}  // namespace pal4
