#include "pal4/pal_actor_control_loop.h"
#include "pal4/pal_actor_control_reset.h"

#include <bit>
#include <initializer_list>
#include <stdexcept>

namespace pal4 {

void IPalActorControlLoopRuntime::SetCursor(std::string_view) {}
void IPalActorControlLoopRuntime::ProcessInput(
    PalActorControlState& state) {
    PalActorControlRuntimeStub runtime;
    static_cast<void>(PalActorControl::ProcessInput(state, 0u, runtime));
}
void IPalActorControlLoopRuntime::ProcessInputAndClearVector(
    PalActorControlState& state) {
    PalActorControlRuntimeStub runtime;
    static_cast<void>(PalActorControl::ProcessInputAndClearVector(
        state, 0u, runtime));
}
void IPalActorControlLoopRuntime::InitializeInputState() {}
void IPalActorControlLoopRuntime::ProcessMonsters() {}
void IPalActorControlLoopRuntime::ClearMonsterFlags() {}
void IPalActorControlLoopRuntime::SetActorAnimationFlag(
    std::uint32_t, std::uint32_t) {}
bool IPalActorControlLoopRuntime::DialogNeedsUpdate(std::uint32_t* dialog) {
    if (dialog[1] != 0u)
        throw std::logic_error("PAL4 active actor effect handle is unbound");
    return false;
}
void IPalActorControlLoopRuntime::UpdateDialog(std::uint32_t*) {}
std::uint8_t IPalActorControlLoopRuntime::UpdateDialogResult(
    std::uint32_t* dialog) {
    UpdateDialog(dialog);
    return 0u;
}
void IPalActorControlLoopRuntime::UpdateGameStateConditional(
    PalActorControlState&, std::uint32_t) {}
void IPalActorControlLoopRuntime::UpdateGameStateAndUi(
    PalActorControlState&, std::uint32_t) {}
void IPalActorControlLoopRuntime::UpdateDialogStatus(
    PalActorControlState&, std::uint32_t) {}
std::uint32_t IPalActorControlLoopRuntime::GetPetManager() { return 0u; }
bool IPalActorControlLoopRuntime::PetActive(std::uint32_t) { return false; }
std::uint32_t IPalActorControlLoopRuntime::GetArenaManager() { return 0u; }
void IPalActorControlLoopRuntime::ScriptPetOnRemove(std::uint32_t) {}
void IPalActorControlLoopRuntime::PetVirtualRemove(std::uint32_t) {}
std::uint32_t IPalActorControlLoopRuntime::ResetPet(std::uint32_t) {
    return 0u;
}
void IPalActorControlLoopRuntime::PetVirtualLoad(
    std::uint32_t, std::string_view, std::string_view, std::uint32_t) {}
void IPalActorControlLoopRuntime::InitializePetData(
    std::uint32_t, std::uint32_t) {}
void IPalActorControlLoopRuntime::SetPetStateFlag(
    std::uint32_t, std::uint32_t) {}
std::uint32_t IPalActorControlLoopRuntime::ScriptPetOnAdd(
    std::uint32_t) { return 0u; }

std::uint32_t PalActorControlLoop::ResetAndSync(
    PalActorControlState& state, bool clear_monster_flags,
    IPalActorControlLoopRuntime& runtime) {
    return ResetActorControlState(state, clear_monster_flags, runtime);
}

std::uint8_t PalActorControlLoop::ClearActor(
    PalActorControlState& state,
    IPalActorControlLoopRuntime& runtime) {
    auto& words = state.words;
    runtime.ProcessInput(state);
    runtime.ProcessInputAndClearVector(state);
    runtime.InitializeInputState();
    for (const auto index : {188u, 195u, 202u})
        words[index] = 0u;
    if (runtime.DialogNeedsUpdate(words.data() + 203u))
        runtime.UpdateDialog(words.data() + 203u);
    for (const auto index : {1u, 6u, 183u, 184u, 185u})
        words[index] = 0u;
    if (runtime.DialogNeedsUpdate(words.data() + 186u))
        runtime.UpdateDialog(words.data() + 186u);
    for (const auto index : {20u, 21u, 22u, 223u, 224u})
        words[index] = 0u;
    if (runtime.DialogNeedsUpdate(words.data() + 279u))
        return runtime.UpdateDialogResult(words.data() + 279u);
    return 0u;
}

void PalActorControlLoop::ResetPlayerToJumpStart(
    PalActorControlState& state, const std::uint32_t event_context,
    IPalActorControlLoopRuntime& runtime) {
    if (state.words[1u] != 0u) {
        runtime.ResetPlayerDatabase(
            state.words.data() + 35u, state.words[1u], event_context);
    }
}

void IPalActorControlLoopRuntime::ResetPlayerDatabase(
    std::uint32_t*, std::uint32_t, std::uint32_t) {}

}  // namespace pal4
