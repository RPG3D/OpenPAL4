#include "pal4/game_object_lifecycle.h"
#include "pal4/game_object_behavior_setup.h"

namespace pal4 {

void GameObjectLifecycleRuntimeStub::CleanupEventsAndScripts(
    GameObjectState& state, std::uint32_t address, std::uint32_t mode) {
    GameObjectBehaviorSetupRuntimeStub runtime;
    (void)GameObjectBehaviorSetup::CleanupEventsAndScripts(
        state, address, mode, runtime);
}
std::uint32_t GameObjectLifecycleRuntimeStub::Reset(
    GameObjectState&, std::uint32_t) { return 0u; }
std::uint32_t GameObjectLifecycleRuntimeStub::UpdateState(
    GameObjectState&, std::uint32_t, std::uint32_t) { return 0u; }
void GameObjectLifecycleRuntimeStub::HandleEventProcessing(
    std::uint32_t, float, std::uint32_t, std::uint32_t, std::uint32_t) {}
std::uint32_t GameObjectLifecycleRuntimeStub::UpdatePosition(
    GameObjectState&, std::uint32_t, float, std::uint32_t) { return 0u; }

std::uint32_t GameObjectLifecycle::CleanupVirtual(
    GameObjectState& state, std::uint32_t address,
    IGameObjectLifecycleRuntime& runtime) {
    runtime.CleanupEventsAndScripts(state, address, 1u);
    state.words[kStatusOffset / 4u] = 0u;
    return runtime.Reset(state, address);
}

std::uint32_t GameObjectLifecycle::ConditionalProcess(
    GameObjectState& state, std::uint32_t address,
    std::uint32_t argument, IGameObjectLifecycleRuntime& runtime) {
    if (state.words[kActiveOffset / 4u] == 0u) return 0u;
    return runtime.UpdateState(state, address, argument);
}

std::uint32_t GameObjectLifecycle::HandleEventAndProcess(
    GameObjectState& state, std::uint32_t address,
    float delta_time, std::uint32_t argument,
    IGameObjectLifecycleRuntime& runtime) {
    if (state.words[kActiveOffset / 4u] == 0u) return 0u;
    runtime.HandleEventProcessing(address + kProcessorOffset, delta_time,
                                  address + kScriptOffset,
                                  address + kTreeOffset,
                                  address + kContextOffset);
    return runtime.UpdatePosition(state, address, delta_time, argument);
}

}  // namespace pal4
