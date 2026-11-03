#include "pal4/game_object_behavior_setup.h"
#include "pal4/game_object_npc_behavior_config.h"

namespace pal4 {

std::uint32_t GameObjectBehaviorSetupRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
void GameObjectBehaviorSetupRuntimeStub::ClearProcessorEvents(
    std::uint32_t) {}
void GameObjectBehaviorSetupRuntimeStub::CleanupScriptCollection(
    std::uint32_t) {}
void GameObjectBehaviorSetupRuntimeStub::SetProcessorValue(
    std::uint32_t, std::uint32_t) {}
void GameObjectBehaviorSetupRuntimeStub::SetProcessorFlag(
    std::uint32_t, std::uint32_t) {}
std::uint32_t GameObjectBehaviorSetupRuntimeStub::GetChildByIndex(
    std::uint32_t, std::uint32_t&) { return 0u; }
void GameObjectBehaviorSetupRuntimeStub::ProcessNpcBehaviorConfig(
    GameObjectState&, std::uint32_t address, std::uint32_t child) {
    GameObjectNpcBehaviorRuntimeStub runtime;
    GameObjectNpcBehaviorConfig::Process(address, child, runtime);
}
std::uint32_t GameObjectBehaviorSetupRuntimeStub::CallObjectVirtual(
    GameObjectState&, std::uint32_t, std::uint32_t,
    std::uint32_t, std::uint32_t) { return 0u; }

namespace {

void SetupBehavior(GameObjectState& state, std::uint32_t address,
                   std::uint32_t npc_record, std::uint32_t attach_fields,
                   bool clear_previous,
                   IGameObjectBehaviorSetupRuntime& runtime) {
    state.words[GameObjectLifecycle::kStatusOffset / 4u] = npc_record;
    if (npc_record == 0u) return;

    const auto document = runtime.ReadDword(
        npc_record + GameObjectBehaviorSetup::kBehaviorDocumentOffset);
    if (document == 0u) return;

    const auto processor = address + GameObjectLifecycle::kProcessorOffset;
    if (clear_previous) {
        runtime.ClearProcessorEvents(processor);
        runtime.CleanupScriptCollection(
            address + GameObjectLifecycle::kScriptOffset);
    }
    runtime.SetProcessorValue(processor, GameObjectBehaviorSetup::kTaskReturnTrue);
    runtime.SetProcessorFlag(processor, 1u);
    std::uint32_t index = 0u;
    const auto child = runtime.GetChildByIndex(document, index);
    state.words[GameObjectBehaviorSetup::kBehaviorChildOffset / 4u] = child;
    runtime.ProcessNpcBehaviorConfig(state, address, child);
    if (attach_fields == 0u) return;

    (void)runtime.CallObjectVirtual(state, address, 4u,
                                    npc_record + 0x24u, 0u);
    (void)runtime.CallObjectVirtual(state, address, 8u,
                                    npc_record + 0x30u, 0u);
}

}  // namespace

void GameObjectBehaviorSetup::Setup(
    GameObjectState& state, std::uint32_t address,
    std::uint32_t npc_record, std::uint32_t attach_fields,
    IGameObjectBehaviorSetupRuntime& runtime) {
    SetupBehavior(state, address, npc_record, attach_fields, false, runtime);
}

void GameObjectBehaviorSetup::ProcessEventsAndClear(
    GameObjectState& state, std::uint32_t address,
    std::uint32_t npc_record, std::uint32_t attach_fields,
    IGameObjectBehaviorSetupRuntime& runtime) {
    SetupBehavior(state, address, npc_record, attach_fields, true, runtime);
}

std::uint32_t GameObjectBehaviorSetup::CleanupEventsAndScripts(
    GameObjectState& state, std::uint32_t address,
    std::uint32_t detach_field,
    IGameObjectBehaviorSetupRuntime& runtime) {
    const auto npc_record = state.words[GameObjectLifecycle::kStatusOffset / 4u];
    if (npc_record == 0u) return 0u;

    runtime.ClearProcessorEvents(
        address + GameObjectLifecycle::kProcessorOffset);
    runtime.CleanupScriptCollection(
        address + GameObjectLifecycle::kScriptOffset);
    std::uint32_t result = detach_field;
    if (detach_field != 0u)
        result = runtime.CallObjectVirtual(
            state, address, 4u, npc_record + 0x24u, 0u);
    state.words[kBehaviorChildOffset / 4u] = 0u;
    return result;
}

}  // namespace pal4
