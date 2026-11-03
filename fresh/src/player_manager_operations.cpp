#include "pal4/player_manager_operations.h"

namespace pal4 {

std::uint32_t PlayerManagerOperationsRuntimeStub::UpdateInventory(
    std::uint32_t, std::uint32_t) { return 0u; }
void PlayerManagerOperationsRuntimeStub::ProcessGameData(std::uint32_t) {}
void PlayerManagerOperationsRuntimeStub::CallScriptIndex(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
std::uint32_t PlayerManagerOperationsRuntimeStub::SetStateFlag(
    std::uint32_t, std::uint32_t) { return 0u; }
void PlayerManagerOperationsRuntimeStub::ApplyMaterialTexture(
    std::uint32_t) {}
std::uint32_t PlayerManagerOperationsRuntimeStub::ApplyMaterialTexture2(
    std::uint32_t) { return 0u; }
void PlayerManagerOperationsRuntimeStub::CallSlotReset(
    std::uint32_t, std::uint32_t) {}
std::uint32_t PlayerManagerOperationsRuntimeStub::ActorControl() {
    return 0u;
}
std::uint32_t PlayerManagerOperationsRuntimeStub::ClearActor(
    std::uint32_t) { return 0u; }
std::uint32_t PlayerManagerOperationsRuntimeStub::ForAllPlayers(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }
void PlayerManagerOperationsRuntimeStub::CallSlotRender(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
void PlayerManagerOperationsRuntimeStub::RenderDebug(std::uint32_t) {}
std::uint32_t PlayerManagerOperationsRuntimeStub::CallSlotUpdate(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }
std::uint32_t PlayerManagerOperationsRuntimeStub::IterateRelated(
    std::uint32_t, std::uint32_t) { return 0u; }
void PlayerManagerOperationsRuntimeStub::CallSlotProcess(
    std::uint32_t, std::uint32_t, float, std::uint32_t) {}
std::uint32_t PlayerManagerOperationsRuntimeStub::UpdateControlFrame(
    std::uint32_t, float) { return 0u; }

std::uint32_t PlayerManagerOperations::ProcessLoadedData(
    std::uint32_t manager, IPlayerManagerOperationsRuntime& runtime) {
    for (std::uint32_t index = 0u;
         index < PlayerManagerObject::kElementCount; ++index) {
        const auto slot = manager + index * PlayerManagerObject::kElementBytes;
        static_cast<void>(runtime.UpdateInventory(slot, 0u));
        runtime.ProcessGameData(slot);
        runtime.CallScriptIndex(slot + kScriptOffset, 0x18u, index);
    }
    return runtime.UpdateInventory(manager, 1u);
}

std::uint32_t PlayerManagerOperations::ResetData(
    std::uint32_t manager, IPlayerManagerOperationsRuntime& runtime) {
    std::uint32_t result = 0u;
    for (std::uint32_t index = 0u;
         index < PlayerManagerObject::kElementCount; ++index)
        result = runtime.SetStateFlag(
            manager + index * PlayerManagerObject::kElementBytes, 0u);
    return result;
}

std::uint32_t PlayerManagerOperations::UpdateData(
    std::uint32_t manager, IPlayerManagerOperationsRuntime& runtime) {
    std::uint32_t result = 0u;
    for (std::uint32_t index = 0u;
         index < PlayerManagerObject::kElementCount; ++index) {
        const auto slot = manager + index * PlayerManagerObject::kElementBytes;
        runtime.ApplyMaterialTexture(slot);
        result = runtime.ApplyMaterialTexture2(slot);
    }
    return result;
}

std::uint32_t PlayerManagerOperations::ResetAllPlayerStates(
    std::uint32_t manager, IPlayerManagerOperationsRuntime& runtime) {
    for (std::uint32_t index = 0u;
         index < PlayerManagerObject::kElementCount; ++index)
        runtime.CallSlotReset(
            manager + index * PlayerManagerObject::kElementBytes, 0x2cu);
    return runtime.ClearActor(runtime.ActorControl());
}

std::uint32_t PlayerManagerOperations::ForAllPlayers(
    std::uint32_t manager, std::uint32_t callback,
    std::uint32_t context, IPlayerManagerOperationsRuntime& runtime) {
    return runtime.ForAllPlayers(manager, callback, context);
}

void PlayerManagerOperations::UpdateAndRender(
    const PlayerManagerObjectState& state, std::uint32_t manager,
    std::uint32_t argument, IPlayerManagerOperationsRuntime& runtime) {
    for (std::uint32_t index = 0u;
         index < PlayerManagerObject::kElementCount; ++index) {
        const auto word = (index * PlayerManagerObject::kElementBytes +
                           kStateFlagOffset) / 4u;
        if (state.Word(word) != 0u)
            runtime.CallSlotRender(
                manager + index * PlayerManagerObject::kElementBytes,
                0x34u, argument);
    }
    runtime.RenderDebug(runtime.ActorControl());
}

std::uint32_t PlayerManagerOperations::ProcessPlayerUpdates(
    const PlayerManagerObjectState& state, std::uint32_t manager,
    std::uint32_t argument, IPlayerManagerOperationsRuntime& runtime) {
    std::uint32_t result = 0u;
    for (std::uint32_t index = 0u;
         index < PlayerManagerObject::kElementCount; ++index) {
        result = state.Word((index * PlayerManagerObject::kElementBytes +
                             kStateFlagOffset) / 4u);
        if (result != 0u)
            result = runtime.CallSlotUpdate(
                manager + index * PlayerManagerObject::kElementBytes,
                0x24u, argument);
    }
    return result;
}

std::uint32_t PlayerManagerOperations::ProcessPlayerRelatedData(
    const PlayerManagerObjectState& state, std::uint32_t manager,
    std::uint32_t argument, IPlayerManagerOperationsRuntime& runtime) {
    std::uint32_t result = 0u;
    for (std::uint32_t index = 0u;
         index < PlayerManagerObject::kElementCount; ++index) {
        result = state.Word((index * PlayerManagerObject::kElementBytes +
                             kStateFlagOffset) / 4u);
        if (result != 0u)
            result = runtime.IterateRelated(
                manager + index * PlayerManagerObject::kElementBytes +
                    kRelatedOffset,
                argument);
    }
    return result;
}

std::uint32_t PlayerManagerOperations::ProcessGameElements(
    const PlayerManagerObjectState& state, std::uint32_t manager,
    float elapsed, std::uint32_t argument,
    IPlayerManagerOperationsRuntime& runtime) {
    for (std::uint32_t index = 0u;
         index < PlayerManagerObject::kElementCount; ++index) {
        const auto word = (index * PlayerManagerObject::kElementBytes +
                           kStateFlagOffset) / 4u;
        if (state.Word(word) != 0u)
            runtime.CallSlotProcess(
                manager + index * PlayerManagerObject::kElementBytes,
                0x38u, elapsed, argument);
    }
    return runtime.UpdateControlFrame(runtime.ActorControl(), elapsed);
}

}  // namespace pal4
