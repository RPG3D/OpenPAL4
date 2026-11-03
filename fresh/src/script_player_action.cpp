#include "pal4/script_player_action.h"

#include <string>

namespace pal4 {
namespace {

char FoldAscii(char value) {
    return value >= 'A' && value <= 'Z' ?
        static_cast<char>(value + ('a' - 'A')) : value;
}

bool SameAction(std::string_view left, std::string_view right) {
    if (left.size() != right.size()) return false;
    for (std::size_t i = 0; i < left.size(); ++i)
        if (FoldAscii(left[i]) != FoldAscii(right[i])) return false;
    return true;
}

bool UsesEquipmentVisuals(std::string_view action) {
    return SameAction(action, "Z01") || SameAction(action, "Z03") ||
           SameAction(action, "Z05") || SameAction(action, "J31");
}

}  // namespace

std::uint32_t ScriptPlayerAction::StartIndexed(
    std::int32_t index, std::string_view action,
    std::int32_t execute_flag, std::uint8_t wait_mode) {
    const auto manager = runtime_.PlayerManager();
    return Start(runtime_.PlayerByIndex(manager, index), action,
                 execute_flag, wait_mode, false, {});
}

std::uint32_t ScriptPlayerAction::StartCurrent(
    std::string_view action, std::int32_t execute_flag,
    std::uint8_t wait_mode) {
    return Start(runtime_.CurrentPlayer(runtime_.PlayerManager()), action,
                 execute_flag, wait_mode, false, {});
}

std::uint32_t ScriptPlayerAction::StartNpc(
    std::string_view npc_name, std::string_view action,
    std::int32_t execute_flag, std::uint8_t wait_mode) {
    const auto actor = runtime_.NpcByName(runtime_.NpcManager(), npc_name);
    return Start(actor, action, execute_flag, wait_mode, true, npc_name);
}

std::uint32_t ScriptPlayerAction::StartRepeatIndexed(
    std::int32_t index, std::string_view action) {
    const auto manager = runtime_.PlayerManager();
    return Start(runtime_.PlayerByIndex(manager, index), action,
                 1, 0u, false, {}, true);
}

std::uint32_t ScriptPlayerAction::StartRepeatNpc(
    std::string_view npc_name, std::string_view action) {
    const auto manager = runtime_.NpcManager();
    return Start(runtime_.NpcByName(manager, npc_name), action,
                 1, 0u, true, npc_name, true);
}

std::uint32_t ScriptPlayerAction::Start(
    std::uint32_t actor, std::string_view action,
    std::int32_t execute_flag, std::uint8_t wait_mode,
    bool npc, std::string_view npc_name, bool repeat) {
    if (actor == 0u) return 0u;
    static_cast<void>(runtime_.SetAction(actor, action));
    if (repeat)
        runtime_.UpdateEffectList(actor);
    else
        runtime_.SetFlag(actor, 1u);
    if (!npc) {
        if (UsesEquipmentVisuals(action))
            runtime_.ApplyEquipmentVisuals(actor);
        else
            runtime_.ClearWeaponEffects(actor);
    }

    const auto name = npc ? std::string(npc_name) + "-Player-Movment" :
                            std::string(runtime_.FormatActorName(actor));
    auto* state = runtime_.AllocateAction(sizeof(PlayerActionWaitState));
    if (state != nullptr) {
        runtime_.ConstructAction(*state, wait_mode,
                                 runtime_.ScriptExecutor() + kQueueOffset);
        state->execute_flag = execute_flag;
        state->completed_loops = 0u;
        state->actor = actor;
        state->vtable_address = kActionVtable;
        state->phase = 3u;
        runtime_.AssignName(*state, name);
        if (execute_flag == -1) runtime_.UpdateEffectList3(actor);
        if (execute_flag == 1) runtime_.SetFlag(actor, 0u);
        runtime_.FlushAction(*state);
    }
    runtime_.AddAction(runtime_.ScriptExecutor() + kQueueOffset, state);
    return runtime_.CleanupName();
}

void ScriptPlayerAction::ResetAction(PlayerActionWaitState& state) {
    state.completed_loops = 0u;
    runtime_.CloseAction(state);
}

void ScriptPlayerAction::UpdateAction(PlayerActionWaitState& state) {
    if (state.execute_flag == -1) {
        if (SameAction(runtime_.ActionName(state.actor), "hold")) {
            state.completed_loops = 0u;
            runtime_.UpdateEffectList4(state.actor);
            runtime_.CloseAction(state);
        }
        return;
    }
    if (state.execute_flag <= 0) return;
    if (state.completed_loops <
        static_cast<std::uint32_t>(state.execute_flag)) {
        if (runtime_.GetFlag(state.actor)) ++state.completed_loops;
        return;
    }
    if (runtime_.SetAction(state.actor, "C01") == 0)
        static_cast<void>(runtime_.SetAction(state.actor, "Z01"));
    if (state.execute_flag == 1) runtime_.SetFlag(state.actor, 1u);
    state.completed_loops = 0u;
    runtime_.CloseAction(state);
}

std::uint32_t ScriptPlayerAction::EndIndexed(std::int32_t index) {
    const auto manager = runtime_.PlayerManager();
    return End(runtime_.PlayerByIndex(manager, index));
}

std::uint32_t ScriptPlayerAction::EndCurrent() {
    return End(runtime_.CurrentPlayer(runtime_.PlayerManager()));
}

std::uint32_t ScriptPlayerAction::EndNpc(std::string_view npc_name) {
    return End(runtime_.NpcByName(runtime_.NpcManager(), npc_name));
}

std::uint32_t ScriptPlayerAction::EndRepeatIndexed(std::int32_t index) {
    const auto manager = runtime_.PlayerManager();
    return EndRepeat(runtime_.PlayerByIndex(manager, index));
}

std::uint32_t ScriptPlayerAction::EndRepeatNpc(
    std::string_view npc_name) {
    const auto manager = runtime_.NpcManager();
    return EndRepeat(runtime_.NpcByName(manager, npc_name));
}

std::uint32_t ScriptPlayerAction::EndRepeat(std::uint32_t actor) {
    if (actor == 0u) return 0u;
    runtime_.UpdateEffectList2(actor);
    return End(actor);
}

std::uint32_t ScriptPlayerAction::End(std::uint32_t actor) {
    if (actor == 0u) return 0u;
    auto* state = runtime_.AllocateEnd(sizeof(PlayerEndActionWaitState));
    if (state != nullptr) {
        runtime_.ConstructEnd(*state, 1u,
                              runtime_.ScriptExecutor() + kQueueOffset);
        state->actor = actor;
        state->vtable_address = kEndVtable;
        runtime_.FlushEnd(*state);
        runtime_.ClearFlag(actor, 0u);
    }
    return runtime_.AddEnd(runtime_.ScriptExecutor() + kQueueOffset, state);
}

void ScriptPlayerAction::UpdateEnd(PlayerEndActionWaitState& state) {
    if (!runtime_.GetFlag(state.actor)) return;
    if (runtime_.SetAction(state.actor, "C01") == 0)
        static_cast<void>(runtime_.SetAction(state.actor, "Z01"));
    runtime_.SetFlag(state.actor, 1u);
    runtime_.CleanupWeaponEffects(state.actor);
    runtime_.CloseEnd(state);
}

void ScriptPlayerAction::NoOp() {}

}  // namespace pal4
